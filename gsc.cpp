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
  std::cout <<  "\nUsage: " << prog << " <session-file>\n" << std::endl;
}

void print_help( std::ostream& out )
{
  std::cout << "\n"
            << "This is a small utility for running guided shell scripts. Lines from the script are loaded, but will not be executed\n"
            << "until the user preses <Enter>. This is useful if you need to give a command line demo, but don't want to run the demo 'live'.\n" << std::endl;
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

template<typename... Args>
std::string fail(int rc, Args... args)
{

  exit(rc);
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
int pause_max = 4;
void rand_pause()
{
  pause( pause_min + (pause_max-pause_min)*(double)rand()/(double)RAND_MAX );
}



int main(int argc, char *argv[])
{
  int masterfd, slavefd;  // master and slave file descriptors
  char *slavedevice; // slave filename
  pid_t slavePID, ioPID; // PIDs for calls to fork.
  struct termios orig_term_settings; // Saved terminal settings
  struct termios new_term_settings; // Current terminal settings
  struct winsize window_size; // the terminal window size

  int rc; // return codes for C functions
  int nc; // num chars
  char input[BUFSIZ];     // input buffer
  std::string session_file;

  // Check arguments
  if (argc <= 1)
  {
    print_usage(std::string(argv[0]), std::cerr);
    print_help(std::cerr);
    exit(1);
  }

  session_file = argv[1];

  // OPTIONS




  masterfd = posix_openpt(O_RDWR);

  if( masterfd < 0
  ||  grantpt(masterfd) != 0
  ||  unlockpt(masterfd) != 0
  ||  (slavedevice = ptsname(masterfd)) == NULL)
  {
    std::cerr << "Error (" << errno << ") could not setup master side of pty" << std::endl;
    close(masterfd);
    return 1;
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
  {
    std::cerr << "Error (" << errno << ") could not fork slave process" << std::endl;
    close(masterfd);
    close(slavefd);
    return 1;
  }

  if( slavePID != 0 ) // parent process gets child PID. child gets 0.
    ioPID = fork();

  if( ioPID == -1)
  {
    std::cerr << "Error (" << errno << ") could not fork io process" << std::endl;
    close(masterfd);
    close(slavefd);
    return 1;
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

    // Execution of the program
    {
      std::string shell = getenv("SHELL") == NULL ? "/bin/sh" : getenv("SHELL");
      execl(shell.c_str(), strrchr(shell.c_str(), '/') + 1, "-i", (char *)0);
    }

    // if Error...
    return 1;
  }






#define PROCESS_COMMANDS \
    command = ""; \
    ss >> command; \
 \
    if( command == "sim" ) \
      command = "simulate_typing"; \
    if( command == "int" ) \
      command = "interactive"; \
    if( command == "pass" ) \
      command = "passthrough"; \
 \
    if( command == "simulate_typing" ) \
    { \
      ss >> command; \
      if(command == "off") \
        simulate_typing = false; \
      if(command == "on") \
        simulate_typing = true; \
    } \
    if( command == "interactive" ) \
    { \
      ss >> command; \
      if(command == "off") \
        interactive = false; \
      if(command == "on") \
        interactive = true; \
    } \
    if( command == "pause" ) \
    { \
      int count; \
      ss >> count; \
      pause(count); \
    } \
    if( command == "passthrough" ) \
    { \
      input[1] = 0; \
      while( read(0, input, 1) > 0 ) \
      { \
        write(masterfd, input, 1 );  \
      } \
    } \






  // this proc talks to the master, so we
  // don't need the slave fd
  close(slavefd);

  if( ioPID == 0 ) // child that will do IO
  {
    // setup io monitor on master.
    // we want to write any thing we recieve to stdout
    //
    fd_set fd_in;

    while (1)
    {
      FD_ZERO(&fd_in);
      FD_SET(0, &fd_in);
      FD_SET(masterfd, &fd_in);

      rc = select(masterfd + 1, &fd_in, NULL, NULL, NULL);
      switch(rc)
      {
        case -1 : fprintf(stderr, "Error %d on select()\n", errno);
                  exit(1);

        default :
        {
          // If data on master side of PTY
          if (FD_ISSET(masterfd, &fd_in))
          {
            rc = read(masterfd, input, sizeof(input));
            if (rc > 0)
            {
              // Send data on standard output
              write(1, input, rc);
            }
            else
            {
              if (rc < 0)
              {
                fprintf(stderr, "Error %d on read masterfd PTY\n", errno);
                exit(1);
              }
            }
          }
        }
      } // End switch
    } // End while

  }


  if( ioPID && slavePID ) // parent process. will read and run the session file.
  {  
    // disable echo on stdin
    rc = tcgetattr(0, &orig_term_settings);
    new_term_settings = orig_term_settings;
    new_term_settings.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &new_term_settings);




    // Read in session file.
    std::string line;
    const char *linep;
    std::vector<std::string> lines;
    std::ifstream in( session_file.c_str() );
    while (std::getline(in, line))
      lines.push_back( line );


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
        ss.str( std::regex_replace( line, comment_regex, "" ) );
        ss.clear();
        
        PROCESS_COMMANDS

      }

      // skip comments
      if( std::regex_search( line, comment_regex ) )
          continue;


      linep = line.c_str();

      for( j = 0; j < line.size(); j++)
      {
        write(masterfd, linep+j, 1);
        if(simulate_typing)
        {
          rand_pause();
        }
      }

      if(interactive)
      {
        nc = read(0, input, BUFSIZ);
        input[nc-1] = 0;

        ss.str( input );
        ss.clear();
        PROCESS_COMMANDS
      }

      write(masterfd, "\r", 1);

    }
  }

  // make sure child procs are killed
  kill( ioPID, SIGKILL );
  kill( slavePID, SIGKILL );


  return 0;
}


