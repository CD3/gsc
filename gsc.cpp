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

#include <regex>


template<typename T>
std::string toString( T v )
{
  std::stringstream ss;
  ss << v;
  return ss.str();
}


void print_usage(std::string prog, std::ostream& out)
{
  std::cout <<  "\nUsage: " << prog << " [options] <session-file>\n"
            "\n"
            "Options:\n"
            "\n"
            "-i (0|1)    Disable or allow interactive mode. If '-i 0' is given, interactive mode will not be enabled, even if a control command (see below) turns it on.\n"
            "            If '-i 1' is given, interactive mode can still be disabled with a control command.\n"
            "-s (0|1)    Disable or allow typing simulation mode. If '-s 0' is given, simulation mode will not be enabled, even if a control command (see below) turns it on.\n"
            "            If '-s 1' is given, simulation mode can still be disabled with a control command.\n"
            "\n"
            << std::endl;
}

void print_help( std::ostream& out )
{
  std::cout << "\n"
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
               "The behavior of gsc can be changed on the fly with control commands.\n"
               "These commands are either entered by the user at the keyboard\n"
               "(they will not be echoed to the screen) or placed in a comment line of\n"
               "the session file. In either case, the command syntax is the same.\n"
               "For example, this line\n"
               "\n"
               "   # interactive off\n"
               "\n"
               "is equivalent to the user typing 'interactive off'.\n"
               "\n"
               "  Supported Commands:\n"
               "\n"
               "  interactive (on|off)      Turn interactive mode on/off.\n"
               "  simulate_typing (on|off)  Turn typing simulation mode on/off.\n"
               "  pause COUNT               Pause for COUNT tenths of a second ('pause 5' will pause for one half second).\n"
               "  passthrough               Enable passthrough mode. All user input will be passed directly to the terminal until Ctrl-D.\n"
               "  stdout (on|off)           Turn stdout of the shell process on/off. This allows you to run some commands silently.\n"
               "\n"
               "  Several short versions of each command are supported."
               "\n"
               "  int   -> interactive\n"
               "  sim   -> simulate_typing\n"
               "  pass  -> passthrough\n"
               "\n"
               "Modes:\n"
               "\n"
               "gsc supports different modes of operation. Modes are not mutually exclusive, more than one mode can be active at one time.\n"
               "\n"
               "interactive mode           The user must hit <Enter> to execute commands.\n"
               "typing simulation mode     Characters are loaded into the command line one at a time with a short pause between each to\n"
               "                           simulate typing. This is useful in demos to give your audience time to process the command\n"
               "                           you are demonstrating.\n"
               "typing simulation mode     The user must hit <Enter> to execute commands."
               "\n"

            << std::endl;
}


inline std::string rtrim(
  const std::string& s,
  const std::string& delimiters = " \f\n\r\t\v" )
{
  return s.substr( 0, s.find_last_not_of( delimiters ) + 1 );
}

inline std::string ltrim(
  const std::string& s,
  const std::string& delimiters = " \f\n\r\t\v" )
{
  return s.substr( s.find_first_not_of( delimiters ) );
}

inline std::string trim(
  const std::string& s,
  const std::string& delimiters = " \f\n\r\t\v" )
{
  return trim( trim( s, delimiters ), delimiters );
}

void fail(std::string msg)
{
  std::cerr << "Error (" << errno << "): " << msg << std::endl;
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

int pause_min = 1;
int pause_max = 1;
void rand_pause()
{
  pause( pause_min + (pause_max-pause_min)*(double)rand()/(double)RAND_MAX );
}



int main(int argc, char *argv[])
{
  int masterfd, slavefd;  // master and slave file descriptors
  char *slavedevice; // slave filename
  pid_t slavePID, ioPID; // PIDs for calls to fork.
  int ioPipe[2];
  struct termios orig_term_settings; // Saved terminal settings
  struct termios new_term_settings; // Current terminal settings
  struct winsize window_size; // the terminal window size

  int rc; // return codes for C functions
  int nc; // num chars
  char input[BUFSIZ];     // input buffer
  std::string str;     // string for misc uses
  std::string session_file;


  // OPTIONS
  int opt, hflg=0, iflg=1, sflg=1;
  while((opt = getopt(argc,argv,"hi:s:")) != EOF)
  switch(opt)
  {
    case 'h': hflg=1; break;
    case 'i': iflg=atoi(optarg); break;
    case 's': sflg=atoi(optarg); break;
    case '?': std::cerr << "ERROR: Unrecognized option '-" << opt <<"'\n";
              print_help(std::cerr);
              exit(1);
  }

  // Check arguments
  if(hflg || optind >= argc)
  {
    print_usage(std::string(argv[0]), std::cerr);
    print_help(std::cerr);
    exit(1);
  }


  session_file = argv[optind];


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
  // 1. a shell is executed and attached to to the slave device (i.e. its standard input/output/error are directed at teh slave)
  // 2. lines from the session file are read and then written to the masterfd.
  // 3. ouput from the slave is read from the masterfd and written to stdout.
  //
  // So, we will do each of these in their own porcess.
  
  slavePID = 1;
  ioPID = 1;

  slavePID = fork();
  if( slavePID == -1)
    fail("fork slave proc");

  if( slavePID != 0 ) // parent process gets child PID. child gets 0.
  {
    // we don't need the slavefd in this proc
    close(slavefd);

    // open a pipe that we will use to send commands to the io proc
    if( pipe(ioPipe) == -1)
      fail("io pipe open");

    // create proc to do io
    ioPID = fork();

    if( ioPID == -1)
      fail("fork io proc");
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
      std::string shell = getenv("SHELL") == NULL ? "/bin/sh" : getenv("SHELL");
      execl(shell.c_str(), strrchr(shell.c_str(), '/') + 1, "-i", (char *)0);
    }

    // if we are here, there was an error
    //
    fail("execl");
  }



  if( ioPID == 0 ) // child that will write slaves stdout to actual stdout
  {
    // setup io monitor on masterfd that will write slave output to stdout.
    // we want to write anything we recieve to stdout

    // close the write end of the pip
    if(close(ioPipe[1]) == -1)
      fail("closing io pipe write end");

    fd_set fd_in;
    bool talk = true;

    while (1)
    {
      FD_ZERO(&fd_in);
      FD_SET(masterfd, &fd_in);
      FD_SET(ioPipe[0], &fd_in);

      rc = select(ioPipe[0]+1, &fd_in, NULL, NULL, NULL);
      switch(rc)
      {
        case -1 : fprintf(stderr, "Error %d on select()\n", errno);
                  exit(1);

        default : // one or more file descriptors have input
        {

          // data on our command pipe
          if (FD_ISSET(ioPipe[0], &fd_in))
          {
            rc = read(ioPipe[0], input, sizeof(input));
            if (rc > 0)
            {
              str = input;
              if( str == "stdout off" )
                talk = false;
              if( str == "stdout on" )
                talk = true;
            }
          }

          // data on master side of PTY
          // read and print to stdout
          if (FD_ISSET(masterfd, &fd_in))
          {
            rc = read(masterfd, input, sizeof(input));
            if (talk && rc > 0)
              write(1, input, rc); // Write data on standard output
          }
        }
      } // End switch
    } // End whileKILL
  }


  if( ioPID && slavePID ) // parent process. will read and run the session file.
  {  

    // close the read end of the pip
    if(close(ioPipe[0]) == -1)
      fail("closing io pipe read end");

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





    // Read in session file.
    std::string line;
    const char *linep;
    std::vector<std::string> lines;
    std::ifstream in( session_file.c_str() );
    while (std::getline(in, line))
      lines.push_back( line );


    std::string commandstr;
    bool simulate_typing = true;
    bool interactive     = true;


    std::stringstream ss;
    std::string command;
    std::regex comment_regex("^[ \t]*#");
    int i, j;
    for( i = 0; i < lines.size(); i++)
    {
      line = rtrim( lines[i] );

      if( std::regex_search( line, comment_regex ) )
      {
        commandstr = std::regex_replace( line, comment_regex, "" );
        #include "process_commands.h"
      }

      // skip comments
      if( std::regex_search( line, comment_regex ) )
          continue;


      linep = line.c_str();

      for( j = 0; j < line.size(); j++)
      {
        write(masterfd, linep+j, 1);
        if(sflg && simulate_typing)
          rand_pause();
      }

      if(iflg && interactive)
      {
        nc = read(0, input, BUFSIZ);
        input[nc-1] = 0;

        commandstr = input;
        #include "process_commands.h"
      }

      write(masterfd, "\r", 1);

    }
  }

  // make sure child procs are killed
  kill( ioPID,    SIGTERM );
  kill( slavePID, SIGTERM );

  tcsetattr(0, TCSANOW, &orig_term_settings);

  return 0;
}


