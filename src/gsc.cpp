/**
 * This program is based off of the code listed in a pty tutorial at http://rachid.koucha.free.fr/tech_corner/pty_pdip.html
 */

#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/regex.hpp>

#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;


bool fileExists(const string& filename)
{
  struct stat buf;
  if (stat(filename.c_str(), &buf) != -1)
    return true;
  return false;
}

void print_usage(string prog, ostream& out)
{
  cout <<  "Usage: " << prog << " [options] <session-file>" << endl;
}

void print_help( ostream& out )
{
  cout << "\n"
               "This is a small utility for running guided shell scripts.\n"
               "Lines from the script are loaded, but will not be executed\n"
               "until the user presses <Enter>. This is useful if you need to\n"
               "give a command line demo, but don't want to run the demo 'live'.\n"
               "\n"
               "Input lines are read from a script file and passed to a pseudoterminal.\n"
               "Comment lines (beginning with '#' are not passed to the terminal,\n"
               "but are parsed for control commands (see below).\n"
               "\n"
               "\n"
               "Control Commands:\n"
               "\n"
               "\tThe behavior of `gsc` can be changed on the fly with control commands.\n"
               "\tControl commands are given in comment lines (lines beginning with a '#') of the session file.\n"
               "\tFor example, this line will disable interactive mode (commands will be loaded and executed without user input).\n"
               "\n"
               "\t   # interactive off\n"
               "\n"
               "\tSupported Commands:\n"
               "\n"
               "\tinteractive (on|off)      Turn interactive mode on/off.\n"
               "\tsimulate_typing (on|off)  Turn typing simulation mode on/off.\n"
               "\tpause COUNT               Pause for COUNT tenths of a second ('pause 5' will pause for one half second) after running each command.\n"
               "\tpassthrough               Enable passthrough mode. All user input will be passed directly to the terminal until Ctrl-D.\n"
               "\tstdout (on|off)           Turn stdout of the shell process on/off. This allows you to run some commands silently.\n"
               "\tinclude \"script.sh\"       Include the contents of script.sh in this script.\n"
               "\n"
               "\tSeveral short versions of each command are supported."
               "\n"
               "\tint   -> interactive\n"
               "\tsim   -> simulate_typing\n"
               "\tpass  -> passthrough\n"
               "\n"
               "\tModes:\n"
               "\n"
               "\t\t`gsc` supports different modes of operation. Modes are not mutually exclusive, more than one mode can be active at one time.\n"
               "\n"
               "\t\tinteractive mode           The user must hit <Enter> to load and execute commands.\n"
               "\t\ttyping simulation mode     Characters are loaded into the command line one at a time with a short pause between each to\n"
               "\t\t                           simulate typing. This is useful in demos to give your audience time to process the command\n"
               "\t\t                           you are demonstrating.\n"
               "\t\tpassthrough mode           The script is paused and input is taken from the user. This is useful if you need to enter a password\n"
               "\t\t                           or want to run a few extra commands in the middle of a script.\n"
               "\n"
               "\n"
               "Keyboard Commands:\n"
               "\n"
               "\tVarious keyboard commands can be given in interactive mode to modify the normal flow of the script:\n"
               "\n"
               "\t\tb : backup       go back one line in the script."
               "\t\ts : skip         skip current line in script."
               "\t\tp : passthrough  enable passthrough mode."
               "\t\tx : exit         stop the demo and exit. cleanup commands will still be ran."
               "\n"

            << endl;
}

inline string rtrim(
  const string& s,
  const string& delimiters = " \f\n\r\t\v" )
{
  auto pos = s.find_last_not_of( delimiters );
  if( pos == string::npos )
    return "";
  return s.substr( 0, pos + 1 );
}

inline string ltrim(
  const string& s,
  const string& delimiters = " \f\n\r\t\v" )
{
  auto pos = s.find_last_not_of( delimiters );
  if( pos == string::npos )
    return "";
  return s.substr( s.find_first_not_of( delimiters ) );
}

inline string trim(
  const string& s,
  const string& delimiters = " \f\n\r\t\v" )
{
  return ltrim( rtrim( s, delimiters ), delimiters );
}

void fail(string msg)
{
  cerr << "Error (" << errno << "): " << msg << endl;
  exit(1);
}

void pause(long counts)
{
  // pause, by calling nanosleep, for a specified number of counts.
  // one count is a tenth of a second.
  long long dt = counts*1e8;
  struct timespec t;
  t.tv_sec  = dt/1000000000;
  t.tv_nsec = dt%1000000000;
  nanosleep(&t, NULL);

  return;
}

int rand_pause_min = 1;
int rand_pause_max = 1;
void rand_pause()
{
  pause( rand_pause_min + (rand_pause_max-rand_pause_min)*(double)rand()/(double)RAND_MAX );
}

string messenger = "file";
void message(string msg)
{
  if(messenger == "file")
  {
    ofstream out(".gsc-messages", ios_base::app);
    out << msg << endl;
    out.close();
  }

  if(messenger == "notify-send")
  {
    string cmd = "notify-send '"+msg+"'";
    system(cmd.c_str());
  }

  if(messenger == "xmessage")
  {
    string cmd = "xmessage '"+msg+"'";
    system(cmd.c_str());
  }
}

string dirname( string path )
{
  string dir;

  std::size_t found = path.find_last_of("/\\");
  if(found == string::npos)
    dir = ".";
  else
    dir = path.substr(0,found);

  return dir;
}

string basename( string path )
{
  string filename;

  std::size_t found = path.find_last_of("/\\");
  filename = path.substr(found+1);

  return filename;
}

string path_join( string a, string b )
{
  return a+'/'+b;
}

vector<string> load_script( string filename )
{
  if(!fileExists(filename))
    fail("Script file does not exists ("+filename+")");
  vector<string> lines;
  string line;
  ifstream in( filename.c_str() );

  boost::regex include_statement("^[ \t]*#[ \t]*include[ \t]+([^ ]*)");
  boost::smatch match;

  while(getline(in, line))
  {
    if( boost::regex_search( line, match, include_statement ) && match.size() > 1 )
    {
      string fn = trim(match.str(1),"\"" );
      auto llines = load_script( path_join(dirname(filename),fn) );
      lines.insert(lines.end(), llines.begin(), llines.end());
    }
    else
    {
      lines.push_back( line );
    }
  }
  in.close();

  return lines;
}

int main(int argc, char *argv[])
{
  int masterfd, slavefd;  // master and slave file descriptors
  char *slavedevice; // slave filename
  pid_t slavePID, outputPID; // PIDs for calls to fork.
  int outputPipe[2];
  struct termios orig_term_settings; // Saved terminal settings
  struct termios new_term_settings; // Current terminal settings
  struct winsize window_size; // the terminal window size

  int rc; // return codes for C functions
  int nc; // num chars
  char input[BUFSIZ];     // input buffer
  string str;     // string for misc uses
  string session_file;


  // OPTIONS

  po::options_description options("Global options");
  options.add_options()
    ("help,h"            , "print help message")
    ("interactive,i"     , po::value<bool>()->default_value("on"), "disable/enable interactive mode")
    ("simulate-typing,s" , po::value<bool>()->default_value("on"), "disable/enable simulating typing")
    ("test,t"            , "run script in non-interactive mode and check for errors.")
    ("pause,p"           , po::value<int>()->default_value(0),     "pause for given number of deciseconds (1/10 second).")
    ("rand_pause_min"    , po::value<int>()->default_value(1),     "minimum pause time during simulated typing.")
    ("rand_pause_max"    , po::value<int>()->default_value(1),     "maximum pause time during simulated typing.")
    ("shell"             , po::value<string>(), "use shell instead of default.")
    ("wait-chars,w"      , po::value<string>()->default_value(""), "list of characters that will cause script to stop and wait for user to press enter.")
    ("setup-command"     , po::value<vector<string>>()->composing(), "setup command(s) that will be ran before the script.")
    ("cleanup-command"   , po::value<vector<string>>()->composing(), "cleanup command(s) that will be ran after the script.")
    ("config"            , po::value<string>(), "config file with options to read.")
    ("preview"           , "write script lines to a file before they are loaded.")
    ("messenger"         , po::value<string>(), "the method to use for messages")
    ("list-messengers"   , "list the supported messengers")
    ("session-file"      , po::value<string>(), "script file to run.")
    ;

  po::positional_options_description args;
  args.add("session-file", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(options).positional(args).run(), vm);
  po::notify(vm);

  string configfn;

  // read options from config file option
  if(vm.count("config"))
  {
    configfn = vm["config"].as<string>();
    if(!fileExists(configfn))
      fail("Config file does not exists ("+configfn+")");

    ifstream ifs(configfn.c_str());
    po::store(po::parse_config_file(ifs, options), vm);
    po::notify(vm);
  }
  // read options from local config file if it exits
  configfn = ".gscrc";
  if(fileExists(configfn))
  {
    ifstream ifs(configfn.c_str());
    po::store(po::parse_config_file(ifs, options), vm);
    po::notify(vm);
  }
  // read options from local config file if it exits
  if( getenv("HOME") != NULL )
  {
    configfn = string(getenv("HOME"))+"/.gscrc";
    if(fileExists(configfn))
    {
      ifstream ifs(configfn.c_str());
      po::store(po::parse_config_file(ifs, options), vm);
      po::notify(vm);
    }
  }



  bool iflg = vm["interactive"].as<bool>();
  bool sflg = vm["simulate-typing"].as<bool>();
  bool hflg = vm.count("help");

  int pause_time = vm["pause"].as<int>();

  if(vm.count("test"))
  {
    iflg = false;
    sflg = false;
    pause_time = 1;
  }

  if( vm.count("rand_pause_min") )
    rand_pause_min = vm["rand_pause_min"].as<int>();
  if( vm.count("rand_pause_max") )
    rand_pause_max = vm["rand_pause_max"].as<int>();

  if( rand_pause_max < rand_pause_min )
    rand_pause_max = rand_pause_min;
  if( rand_pause_min > rand_pause_max )
    rand_pause_min = rand_pause_max;

  if( vm.count("messenger") )
    messenger = vm["messenger"].as<string>();

  if( vm.count("list-messengers") )
  {
    cout << "messenger: \n"
         << "\t file\n"
         ;
  }


  // Check arguments
  if(hflg)
  {
    print_usage(string(argv[0]), cerr);
    cout << options << endl;
    print_help(cerr);
    exit(1);
  }


  session_file = vm["session-file"].as<string>();
  if(!fileExists(session_file))
  {
    fail("Session file does not exists ("+session_file+")");
  }


  // create master side of the PTY
  masterfd = posix_openpt(O_RDWR);
  if( masterfd < 0
  ||  grantpt(masterfd) != 0
  ||  unlockpt(masterfd) != 0
  ||  (slavedevice = ptsname(masterfd)) == NULL)
  {
    fail("pty setup");
  }

  // get the current window size
  ioctl( 0, TIOCGWINSZ, (char *)&window_size );


  // Open the slave side ot the PTY
  slavefd = open(slavedevice, O_RDWR);



  // OK, we have three things that need to happen now.
  //
  // 1. a shell is executed and attached to to the slave device (i.e. its standard input/output/error are directed at the slave)
  // 2. lines from the session file are read and then written to the masterfd.
  // 3. ouput from the slave is read from the masterfd and written to stdout.
  //
  // So, we will do each of these in their own process.
  
  slavePID = 1;
  outputPID = 1;

  slavePID = fork();
  if( slavePID == -1)
    fail("fork slave proc");

  if( slavePID != 0 ) // parent process gets child PID. child gets 0.
  {
    // we don't need the slavefd in this proc
    close(slavefd);

    // open a pipe that we will use to send commands to the output proc
    if( pipe(outputPipe) == -1)
      fail("output pipe open");

    // create proc to do output
    outputPID = fork();

    if( outputPID == -1)
      fail("fork output proc");
  }



  // OK, we have our 3 processes.
  
  if( slavePID == 0 ) // child that will run slave
  {
    // setup the slave to act as the controlling terminal for a shell

    // Close the master side of the PTY
    close(masterfd);

    // Save the defaults parameters of the slave side of the PTY
    rc = tcgetattr(slavefd, &orig_term_settings);

    // Set RAW mode on slave side of PTY
    new_term_settings = orig_term_settings;
    cfmakeraw (&new_term_settings);
    tcsetattr (slavefd, TCSANOW, &new_term_settings);

    // The slave side of the PTY becomes the standard input and outputs of the child process
    close(0); // Close standard input (current terminal)
    close(1); // Close standard output (current terminal)
    close(2); // Close standard error (current terminal)

    dup(slavefd); // PTY becomes standard input (0)
    dup(slavefd); // PTY becomes standard output (1)
    dup(slavefd); // PTY becomes standard error (2)

    // Now we can close the original file descriptor
    close(slavefd);

    // set the window size to match what it was when we started.
    ioctl( 0, TIOCSWINSZ, (char *)&window_size );

    // Make the current process a new session leader
    setsid();

    // As the child is a session leader, set the controlling terminal to be the slave side of the PTY
    // (Mandatory for programs like the shell to make them manage their outputs correctly)
    ioctl(0, TIOCSCTTY, 1);

    // Start the shell
    {
      string shell;
      if( vm.count("shell") )
        shell = vm["shell"].as<string>();
      else
        shell = getenv("SHELL") == NULL ? "/bin/sh" : getenv("SHELL");
      execl(shell.c_str(), strrchr(shell.c_str(), '/') + 1, "-i", (char *)0);
    }

    // if we are here, there was an error
    //
    fail("execl");
  }



  if( outputPID == 0 ) // child that will write slaves stdout to actual stdout
  {
    // setup output monitor on masterfd that will write slave output to stdout.
    // we want to write anything we recieve to stdout

    // close the write end of the pipe
    if(close(outputPipe[1]) == -1)
      fail("closing output pipe write end");

    fd_set fd_in;
    bool talk = true;

    while (1)
    {
      FD_ZERO(&fd_in);
      FD_SET(masterfd, &fd_in);
      FD_SET(outputPipe[0], &fd_in);

      rc = select(outputPipe[0]+1, &fd_in, NULL, NULL, NULL);
      switch(rc)
      {
        case -1 : fprintf(stderr, "Error %d on select()\n", errno);
                  exit(1);

        default : // one or more file descriptors have input
        {

          // data on master side of PTY
          // read and print to stdout
          if (FD_ISSET(masterfd, &fd_in))
          {
            rc = read(masterfd, input, sizeof(input));
            if (talk && rc > 0)
              write(1, input, rc); // Write data on standard output
          }

          // data on our command pipe
          if (FD_ISSET(outputPipe[0], &fd_in))
          {
            rc = read(outputPipe[0], input, sizeof(input));
            if (rc > 0)
            {
              str = input;
              if( str == "stdout off" )
                talk = false;
              if( str == "stdout on" )
                talk = true;
            }
          }

        }
      } // End switch
    } // End whileKILL
  }


  if( outputPID && slavePID ) // parent process. will read and run the session file.
  {  

    // close the read end of the pipe
    if(close(outputPipe[0]) == -1)
      fail("closing output pipe read end");

    // configure terminal for raw mode so we
    // can get chars from the user and send them straight to
    // the slave (by writing to the masterfd).
    //
    // note: we also need to disable echoing since the slave will
    // echo its input.
    rc = tcgetattr(0, &orig_term_settings);
    new_term_settings = orig_term_settings;
    new_term_settings.c_lflag &= ~ECHO;
    new_term_settings.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &new_term_settings);
    if( tcsetattr(0, TCSANOW, &new_term_settings) == -1)
      fail("configuring terminal to non-canonical mode");





    vector<string> lines;
    string line;
    const char *linep;

    string commandstr;
    vector<string> commandtoks;
    stringstream ss;
    string tok;
    boost::regex comment_regex("(?<!\\\\)#.*$");
    boost::regex comment_line_regex("^[ \t]*#");

    int i, j;


    if( vm.count("setup-command") )
    {
    // run setup commands
      write( outputPipe[1], "stdout off", sizeof("stdout off") );
      pause(10);

      for( i = 0; i < vm["setup-command"].as<vector<string>>().size(); i++ )
      {
        line = trim( vm["setup-command"].as<vector<string>>()[i] );
        write(masterfd, line.c_str(), line.size());
        write(masterfd, "\r", 1);
      }

      write( outputPipe[1], "stdout on", sizeof("stdout on") );
      pause(10);
    }




    // Read in session file.
    lines = load_script( vm["session-file"].as<string>() );
    // setup preview file
    ofstream pfs;
    if( vm.count("preview") )
      pfs.open(".gsc-preview");

    bool simulate_typing = true;
    bool interactive     = true;
    bool line_loaded;
    bool exit = false;


    for( i = 0; i < lines.size(); i++)
    {
      pfs << lines[i] << endl;

      line = lines[i];
      // remove comment from line if it exists
      line_loaded=false;
      

      if( boost::regex_search( line, comment_line_regex ) )
      {
        commandstr = boost::regex_replace( line, comment_line_regex, "" );
        commandstr = trim(commandstr);
        commandtoks = boost::split( commandtoks, commandstr, boost::is_any_of(" \t,") );
        #include "process_commands.h"
      }

      // skip comments (could also use an else statement)
      if( boost::regex_search( line, comment_line_regex ) )
          continue;
      // remove any comments from the line
      line = boost::regex_replace( line, comment_regex, "" );
      // replace an escaped hashtags
      line = boost::regex_replace( line, boost::regex("\\\\#"), "#" );



      // require user command before *and* after a line is loaded.
      // before line is loaded...
      #include "handle_interactive_commands.h"
      if(exit)
        break;

      linep = line.c_str();
      for( j = 0; j < line.size(); j++)
      {
        write(masterfd, linep+j, 1);
        if(sflg && simulate_typing)
          rand_pause();
        if( iflg && interactive && strchr(vm["wait-chars"].as<string>().c_str(), linep[j]) != NULL )
          nc = read(0, input, BUFSIZ);

      }
      line_loaded=true;

      // after line is loaded...
      #include "handle_interactive_commands.h"

      write(masterfd, "\r", 1);
      pause( pause_time );
    }

    // close preview file
    pfs << "DONE" << endl;
    pfs.close();

    // wait for the user before exiting unless the exit command was given
    // or we are in non-interactive mode
    if(iflg && interactive && !exit)
      nc = read(0, input, BUFSIZ);

    if( vm.count("cleanup-command") )
    {
      // run cleanup commands
      write( outputPipe[1], "stdout off", sizeof("stdout off") );
      pause(1);
      for( i = 0; i < vm["cleanup-command"].as<vector<string>>().size(); i++ )
      {
        line = trim( vm["cleanup-command"].as<vector<string>>()[i] );
        write(masterfd, line.c_str(), line.size());
        write(masterfd, "\r", 1);
        pause(1);
      }
      pause(10);
    }
  }


  // make sure child procs are killed
  kill( outputPID,    SIGTERM );
  kill( slavePID, SIGTERM );

  tcsetattr(0, TCSANOW, &orig_term_settings);

  return 0;
}


