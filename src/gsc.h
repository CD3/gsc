#ifndef gsc_h
#define gsc_h


// definition of all functions used by gsc.cpp
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

#include <boost/algorithm/string.hpp>

#if USE_XDO
extern "C" {
#include <xdo.h>
}
#endif

using namespace std;


namespace gsc {

inline bool fileExists(const string& filename)
{
  struct stat buf;
  if (stat(filename.c_str(), &buf) != -1)
    return true;
  return false;
}


typedef std::map<std::string, std::string> Context;

inline
std::string fmt( std::string templ, const Context& context, std::string stag="%", std::string etag="%" )
{
  for( auto &c : context )
  {
    std::string pat = stag+c.first+etag;
    boost::regex re(pat);
    templ = boost::regex_replace( templ, re, c.second);
  }

  return templ;
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

inline void fail(string msg)
{
  cerr << "Error (" << errno << "): " << msg << endl;
  exit(1);
}

inline void pause(long counts)
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
inline void rand_pause()
{
  pause( rand_pause_min + (rand_pause_max-rand_pause_min)*(double)rand()/(double)RAND_MAX );
}

string messenger = "file";
inline void message(string msg)
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

inline string dirname( string path )
{
  string dir;

  std::size_t found = path.find_last_of("/\\");
  if(found == string::npos)
    dir = ".";
  else
    dir = path.substr(0,found);

  return dir;
}

inline string basename( string path )
{
  string filename;

  std::size_t found = path.find_last_of("/\\");
  filename = path.substr(found+1);

  return filename;
}

inline string path_join( string a, string b )
{
  return a+'/'+b;
}

inline vector<string> load_script( string filename, Context context = Context() )
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
    line = fmt(line, context);
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

inline void passthrough_char(int masterfd)
{
  char input[2];
  input[1] = 0; // null byte
  if( read(0, input, 1) > 0 )
  {
    if( (int)input[0] == 10 )
      input[0] = '\r'; // replace returns with \r
    write(masterfd, input, 1 ); 
  }
}
// passthrough mode
inline void passthrough( int masterfd )
{
  char input[2];
  input[1] = 0; // null byte
  // read from standard input until EOF (Ctl-D)
  while( read(0, input, 1) > 0 && input[0] != 4)
  {
    if( (int)input[0] == 10 )
      input[0] = '\r'; // replace returns with \r
    write(masterfd, input, 1 ); 
  }
}

}


#endif // include protector
