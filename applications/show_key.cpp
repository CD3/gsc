#include <iostream>
#include <string>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <stdlib.h>

using namespace std;

termios old_ts, new_ts;


void signal_handler(int sig)
{
  tcsetattr(0,TCSANOW,&new_ts);
  exit(EXIT_SUCCESS);
}

template<typename T>
void print(char buffer[],int n)
{
  for(int i = 0; i < n; ++i)
  {
    cout << "'" << static_cast<T>(buffer[i]) << "' ";
  }
}

std::string nl = "\r\n";


template<typename T>
struct interpret
{
  char* b;
  int n;
  interpret(char* bb, int nn)
  :b(bb),n(nn) { }
};


template<typename T>
ostream& operator<<(ostream &out, const interpret<T>& buf)
{
  print<T>(buf.b,buf.n);
  return out;
}


int main(int argc, char *argv[])
{
  if( signal(SIGINT ,signal_handler) == SIG_ERR 
   || signal(SIGQUIT,signal_handler) == SIG_ERR )
  {
    std::cerr << "Could not setup signal handler" << std::endl;
    exit(EXIT_FAILURE);
  }

  int rc;
  char buffer[1024];


  rc = tcgetattr(0,&old_ts);
  new_ts = old_ts;
  cfmakeraw(&new_ts);

  tcsetattr(0,TCSANOW,&new_ts);

  for(;;)
  {
    rc = read(0,&buffer,1023);
    buffer[rc] = '\0';

    cout << "size: " << rc << " chars" << nl;

    std::cout << "representations:" << nl;
    std::cout << "raw: '" << buffer << "'" << nl;
    std::cout << "int: " << interpret<int>(buffer,rc) << nl;
    std::cout << "char: " << interpret<char>(buffer,rc) << nl;

    std::cout << "==================" << nl;

    if( rc == 1 && buffer[0] == '' )
      kill(0,SIGINT);
    if( rc == 1 && buffer[0] == '' )
      kill(0,SIGQUIT);

  }

  tcsetattr(0,TCSANOW,&new_ts);

  return 0;
}
