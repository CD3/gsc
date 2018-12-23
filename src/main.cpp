
#include <iostream>
#include <string>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include "./gsc.h"

namespace po = boost::program_options;
using namespace boost;
using namespace std;

int main(int argc, char *argv[])
{

  // process command line arguments
  po::options_description options("Global options");
  options.add_options()
    ("help,h"            , "print help message")
    ("debug,d"           , "debug mode. print everything.")
    ("interactive,i"     , po::value<bool>()->default_value("on"), "disable/enable interactive mode")
    ("simulate-typing,s" , po::value<bool>()->default_value("on"), "disable/enable simulating typing")
    ("test,t"            , "run script in non-interactive mode and check for errors.")
    ("pause,p"           , po::value<int>()->default_value(0),     "pause for given number of deciseconds (1/10 second) before and after a line is loaded.")
    ("rand_pause_min"    , po::value<int>()->default_value(1),     "minimum pause time during simulated typing.")
    ("rand_pause_max"    , po::value<int>()->default_value(1),     "maximum pause time during simulated typing.")
    ("shell"             , po::value<string>()->default_value(""), "use shell instead of default.")
    ("wait-chars,w"      , po::value<string>()->default_value(""), "list of characters that will cause script to stop and wait for user to press enter.")
    ("setup-command"     , po::value<vector<string>>()->composing(), "setup command(s) that will be ran before the script.")
    ("cleanup-command"   , po::value<vector<string>>()->composing(), "cleanup command(s) that will be ran after the script.")
    ("config-file"       , po::value<vector<string>>()->composing(), "config file(s) with more options to read.")
    ("preview"           , "write script lines to a file before they are loaded.")
    ("messenger"         , po::value<string>(), "the method to use for messages")
    ("list-messengers"   , "list the supported messengers")
    ("context-variable,v", po::value<vector<string>>()->composing(), "add context variable for string formatting.")
    ("save-file"         , po::value<string>(), "write a new session file that contains all commands that were actually ran.")
    ("session-file"      , po::value<string>()->required(), "script file to run.")
    ;

  po::positional_options_description args;
  args.add("session-file", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(options).positional(args).run(), vm);
  po::notify(vm);

  filesystem::path session_path(vm["session-file"].as<string>());
  string session_filename = session_path.filename().string();
  string session_basename = session_path.stem().string();

  if(vm.count("help"))
  {
    cout << options << endl;
    exit(0);
  }

  // initialize logger
  {
    if(vm.count("debug"))
    {
      log::core::get()->set_filter( log::trivial::severity >= log::trivial::debug);
      BOOST_LOG_TRIVIAL(debug) << "Logging level set to 'debug'";
    }
    else
    {
      log::core::get()->set_filter( log::trivial::severity >= log::trivial::error);
    }
  }

  // read additional configuration from file(s)
  {
    vector<string> files;
    files.push_back(session_basename+".gscrc");
    files.push_back(".gscrc");
    files.push_back("%HOME%/.gscrc");
    Context c;
    c["HOME"] = getenv("HOME") == NULL ? "" : getenv("HOME");
    for( auto& file : files )
    {
      string configfile = render( file, c );
      BOOST_LOG_TRIVIAL(debug) << "Checking for '" << configfile << "' to load addition options.";
      if( filesystem::exists(configfile) )
      {
        BOOST_LOG_TRIVIAL(debug) << "\tFound. Loading now.";

        ifstream ifs(configfile.c_str());
        po::store(po::parse_config_file(ifs, options), vm);
        po::notify(vm);

      }
    }
  }





  // create the session that will run the script
  Session session(session_filename,vm["shell"].as<string>());
  // configure the session
  // run the session

  
  try {
    session.run();
  }
  catch(const return_exception& e)
  {
    BOOST_LOG_TRIVIAL(debug) << "Exiting.";
    return 0;
  }
  catch(const std::runtime_error& e)
  {
    BOOST_LOG_TRIVIAL(debug) << "An error occurred. ";
    return 1;
  }
  catch(...)
  {
    BOOST_LOG_TRIVIAL(debug) << "An unknown error occurred.";
    return 1;
  }


  return 0;
}
