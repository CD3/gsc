#include <regex>
#include <string>
#include <chrono>
#include <thread>
#include <iostream>

#include <boost/log/trivial.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

#include <sys/poll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "./gsc.h"


/**
 * Render a template string using a context
 */
std::string render( std::string templ, const Context& context, std::string stag, std::string etag )
{
  for( auto &c : context )
  {
    std::string pat = stag+c.first+etag;
    std::regex re(pat);
    templ = std::regex_replace( templ, re, c.second);
  }

  return templ;
}

void SessionScript::load(const std::string& filename)
{
  if(!boost::filesystem::exists(filename) || boost::filesystem::is_directory(filename))
    throw std::runtime_error("No such file "+filename);

  std::ifstream in(filename.c_str());

  std::string line;
  while( std::getline(in,line) )
  {
    lines.push_back(::render(line, this->context,this->render_stag, this->render_etag));
  }
  in.close();
}
void SessionScript::render()
{
  std::transform( this->lines.begin(), this->lines.end(), this->lines.begin(), [this](std::string& s){ return ::render(s,this->context,this->render_stag, this->render_etag); } );
}

Session::Session(std::string filename, std::string shell)
{
  this->filename = filename;
  this->shell = shell;
  if(this->shell == "")
    this->shell = getenv("SHELL") == NULL ? "sh" : getenv("SHELL");
  this->init_shell_args();



  int rc;
  rc = tcgetattr(0,&terminal_settings);
  if( rc == -1 )
    throw std::runtime_error("Could not retrieve terminal settings on stdin file descriptor.");

  state.terminal_settings = terminal_settings;

  // open a pseudoterminal
  state.masterfd = posix_openpt(O_RDWR);
  // setup the save device
  if( state.masterfd < 0 ) throw std::runtime_error("There was a problem opening pty.");
  BOOST_LOG_TRIVIAL(debug) << "Opened pseudoterminal.";
  BOOST_LOG_TRIVIAL(debug) << "  Master device fd: "<<state.masterfd;
  if( grantpt( state.masterfd ) != 0 ) throw std::runtime_error("Could not grant access to pty slave.");
  if( unlockpt( state.masterfd ) != 0) throw std::runtime_error("Could not unlock slave pty device.");
  state.slave_device_name = ptsname(state.masterfd);
  if(state.slave_device_name == NULL) throw std::runtime_error("Could not get slave pty device name.");
  BOOST_LOG_TRIVIAL(debug) << "  Slave device name: "<<state.slave_device_name;

  state.slavePID = fork();
  if( state.slavePID == -1 ) throw std::runtime_error("Could not fork child process.");

  if(amChild())
  {
    // child doesn't need masterfd
    
    BOOST_LOG_TRIVIAL(debug) << "Closing master fd.";
    close(state.masterfd);
    state.masterfd = -2;

    BOOST_LOG_TRIVIAL(debug) << "Creating new session for child.";
    if( setsid() == -1 ) throw std::runtime_error("Could not start new session.");
    BOOST_LOG_TRIVIAL(debug) << "Opening slave device in child to act as controlling terminal.";
    state.slavefd = open(state.slave_device_name,O_RDWR);
    if(state.slavefd == -1) throw std::runtime_error("Could not open slave device file.");
    BOOST_LOG_TRIVIAL(debug) << "Connecting child stdin, stdou, and stderr to slave device.";
    if( dup2( state.slavefd, 0 ) != 0 ) throw std::runtime_error("Could not connect child stdin to slave device");
    if( dup2( state.slavefd, 1 ) != 1 ) throw std::runtime_error("Could not connect child stdout to slave device");
    if( dup2( state.slavefd, 2 ) != 2 ) throw std::runtime_error("Could not connect child stderr to slave device");
  }

  if(state.slavefd > 0)
  {
    BOOST_LOG_TRIVIAL(debug) << "Closing slave fd.";
    close(state.slavefd);
    state.slavefd = -2;
  }

  if(amChild())
  {
    // launch a shell in the child process
    // need to generate a c-style array of strings
    // to pass arguments to the shell
    auto string2cstr = [](std::string &s)
    {
      return const_cast<char*>(s.c_str());
    };
    std::vector<char*> argv;
    argv.push_back( string2cstr(this->shell) );
    std::transform( this->shell_args.begin(), this->shell_args.end(), std::back_inserter(argv), string2cstr );
    argv.push_back( NULL ); // need to null terminate the array
    BOOST_LOG_TRIVIAL(debug) << "Launching shell (" << this->shell <<") in child process with "<<argv.size()<<" element array for argv:";
    for( auto &a : argv )
    {
      if( a != NULL )
        BOOST_LOG_TRIVIAL(debug) << "  " << a;
      else
        BOOST_LOG_TRIVIAL(debug) << "  NULL";
    }
    execvp( this->shell.c_str(), argv.data() );
  }




}

Session::~Session()
{
  state.shutdown = true;
  close(state.masterfd);
  tcsetattr(0,TCSAFLUSH,&terminal_settings);
}

int Session::run()
{
  BOOST_LOG_TRIVIAL(debug) << "Beginning session run.";

  // set terminal to raw mode
  cfmakeraw(&(state.terminal_settings));
  tcsetattr(0,TCSANOW,&(state.terminal_settings));

  this->script.load( this->filename );
  state.script_line_it = this->script.lines.begin();

  
  if(amParent())
  {
    // create a thread to process output from the
    // slave device.
    std::thread slave_output_thread(&Session::process_slave_output,this);
    // some vars for processing
    // return codes, input chars, and output chars.
    int rc;
    char ch, ich, och;

    // run setup commands first
    BOOST_LOG_TRIVIAL(debug) << "running setup commands";
    for( auto &l : setup_commands )
    {
      BOOST_LOG_TRIVIAL(debug) << "  setup command: " << l;
      for( auto &c : l )
      {
        send_to_slave(c);
      }
      send_to_slave('\r');
    }

    // process script and user input
    for(; state.script_line_it != this->script.lines.end(); state.script_line_it++)
    {
      std::string& line = *state.script_line_it;
      // process line for commands
      
      // send line to shell
      state.line_status = LineStatus::EMPTY;
      for( state.line_character_it = line.begin(); state.line_character_it != line.end(); state.line_character_it++)
      {
        // process user input
        process_user_input();

        ch = *state.line_character_it;
        // send the char to shell
        send_to_slave(ch);
        state.line_status = LineStatus::INPROCESS;

        if(line.end() - state.line_character_it == 1)
        {
          state.line_status = LineStatus::LOADED;
          process_user_input();
        }

      }
      send_to_slave('\r');
    }

    // run cleanup commands first
    BOOST_LOG_TRIVIAL(debug) << "running cleanup commands";
    for( auto &l : cleanup_commands )
    {
      BOOST_LOG_TRIVIAL(debug) << "  cleanup command: " << l;
      for( auto &c : l )
      {
        send_to_slave(c);
      }
      send_to_slave('\r');
    }

    do { get_from_stdin(ich); }while(ich != '\r');
    // inform the user that the session has ended.
    for( auto c : "\n\n\rSession Finished. Press Enter.\n\r" )
      send_to_stdout(c);
    // wait for user before we quit
    do { get_from_stdin(ich); }while(ich != '\r');


    // tell slave output thread to stop
    state.shutdown = true;
    slave_output_thread.join();

  }

  // go ahead and reset the terminal
  tcsetattr(0,TCSAFLUSH,&terminal_settings);

  
  BOOST_LOG_TRIVIAL(debug) << "Session run completed.";
  return 0;
}

bool Session::amChild()
{
  return state.slavePID == 0;
}

bool Session::amParent()
{
  return state.slavePID > 0;
}

int Session::get_from_stdin(char& c)
{
  return read(0,&c,1);
}

int Session::send_to_stdout(char c)
{
  return write(1,&c,1);
}

int Session::get_from_slave(char& c)
{
  // output of slave is read from master input
  return read(state.masterfd,&c,1);
}

int Session::send_to_slave(char c)
{
  // some translation
  if( c == '\n' )
    c = '\r';

  return write(state.masterfd,&c,1);
}

void Session::init_shell_args()
{
  if( boost::algorithm::ends_with( this->shell, "bash" )
  ||  boost::algorithm::ends_with( this->shell, "zsh" )
  ||  boost::algorithm::ends_with( this->shell, "sh" )
  )
  {
    shell_args.push_back("-i");
  }
}

void Session::process_slave_output()
{
  char ch;
  int rc;
  // use a poll to check for data from the slave
  pollfd polls;
  polls.fd = state.masterfd;
  polls.events = POLLIN;
  while(!state.shutdown)
  {
    while( rc = poll(&polls,1,0) )
    {
      if(rc < 0)
        throw std::runtime_error("There was a problem polling masterfd.");

      get_from_slave(ch);
      // check if slave output should be printed
      send_to_stdout(ch);
    }
  }
  


  return;
}

void Session::process_user_input()
{
  char c;
  bool cont;
  // this function would probably be
  // better with a goto.
  cont = true;
  while(cont)
  {
    // by default, we loop once.
    cont = false;

    if(state.input_mode == UserInputMode::COMMAND)
    {
      // =command mode=
      // read characters from standard in and interpret
      // them as commands. this allows the user to modify
      // state.

      while(get_from_stdin(c)) // read input until we recognize a command
      {
        if( c == 'q' ) // quit program
          throw return_exception();
        if( c == 'i' ) // switch to insert mode
        {
          state.input_mode = UserInputMode::INSERT;
          break;
        }
        if( c == 'p' ) // switch to passhthrough mode
        {
          state.input_mode = UserInputMode::PASSTHROUGH;
          break;
        }
        if( c == '\r' ) // return back to caller
        {
          break;
        }
      }
    }

    if(state.input_mode == UserInputMode::INSERT)
    {
      // =insert mode=
      //
      // just return to back to the caller when
      // a key is pressed, UNLESS
      //
      // - the user presses Esc. then switch to command mode and start over
      // - a shell line has been loaded. then wait for the user to press Return
      // - the user presses Backspace. then pass Backspace to the shell and back up the line_char_it
      for(;;) // start looping
      {
          get_from_stdin(c); // reach char from user
          if( c == 127 ) // Backspace key: send to shell, rewind the line character iterator, and start over
          {
            if(state.line_character_it > state.script_line_it->begin())
            { // only delete characters if at least one is loaded.
              send_to_slave(c);
              state.line_character_it--;
              state.line_status = LineStatus::INPROCESS;
            }
            if(state.line_character_it == state.script_line_it->begin())
            {
              state.line_status = LineStatus::EMPTY;
            }
            cont = true;
            break;
          }
          if( c == 27 ) // Esc key: switch to command mode and start over
          {
            state.input_mode = UserInputMode::COMMAND;
            cont = true;
            break;
          }

          if( c == '\r' ) // return back to caller
          {
            break;
          }

          // if a line has been loaded, don't return unless
          // the user presses Enter (which is handled above)
          if( state.line_status == LineStatus::LOADED )
            continue;

          break;

      }
    }

    if(state.input_mode == UserInputMode::PASSTHROUGH)
    {
      // =passthrough mode=
      // 
      // pass user input to the shell until
      // they press Ctl-D. then switch to command mode
      // and start over.
      // TODO: if the user enters passthrough mode while
      // a shell line is still being loaded, they probably don't want
      // to finish the line. should probably signal that the rest of
      // the line should be discarded. perhaps we could set the
      // line status to loaded.
      //
      // on the other hand, the user could switch to passthrough
      // mode to fix a typo and then want the rest of the line to
      // finish loading.
      while(get_from_stdin(c))
      {
        if( iscntrl( (unsigned char)c ) )
        {
          if((c^64)==68)
          {
            state.input_mode = UserInputMode::COMMAND;
            cont = true;
            break;
          }
          
        }
        send_to_slave(c);
      }
    }
  }

  return;
}
