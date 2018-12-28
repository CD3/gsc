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
#include <boost/process.hpp>

#include "./gsc.h"

namespace po = boost::program_options;
using namespace boost;
using namespace std;

std::string manual_text = R"(

)";

int main(int argc, char *argv[])
{

  // process command line arguments
  po::options_description options("Global options");
  options.add_options()
    ("help,h"            , "print help message")
    ("debug,d"           , "debug mode. print everything.")
    ("shell"             , po::value<string>()->default_value(""), "use shell instead of default.")
    ("setup-script"      , po::value<vector<string>>()->composing(), "may be given multiple times. executables that will be ran before the session starts.")
    ("cleanup-script"    , po::value<vector<string>>()->composing(), "may be given multiple times. executable that will be ran after the session finishes.")
    ("setup-command"     , po::value<vector<string>>()->composing(), "may be given multiple times. command that will be passed to the session shell before any script lines.")
    ("cleanup-command"   , po::value<vector<string>>()->composing(), "may be given multiple times. command that will be passed to the session shell before any script lines.")
    ("context-variable,v", po::value<vector<string>>()->composing(), "add context variable for string formatting.")
    //("save-file"         , po::value<string>(), "write a new session file that contains all commands that were actually ran.")
    ("config-file"       , po::value<vector<string>>()->composing(), "config file to read additional options from.")
    ("log-file"          , po::value<string>(), "log file name.")
    ("session-file"      , po::value<string>(), "script file to run.")
    ;

  po::positional_options_description args;
  args.add("session-file", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(options).positional(args).run(), vm);
  po::notify(vm);

  if(vm.count("help"))
  {
    cout << options << endl;
    exit(0);
  }
  if(vm.count("session-file") == 0)
  {
    cout << "Usage: " << argv[0] << " [OPTIONS] <session-file>" << endl;
    cout << options << endl;
    exit(0);
  }


  filesystem::path session_path(vm["session-file"].as<string>());
  string session_filename = session_path.filename().string();
  string session_basename = session_path.stem().string();

  // initialize logger
  {
    string log_filename = "gsc.log";
    if(vm.count("log-file"))
      log_filename = vm["log-file"].as<string>();
    log::add_file_log(log_filename);
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
    //files.push_back(session_basename+".gscrc");
    if( vm.count("config-file") )
    {
      for( auto &f : vm["config-file"].as<vector<string>>() )
        files.push_back(f);
    }
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

  // collect setup and cleanup scripts
  vector<string> setup_scripts;
  if( vm.count("setup-script") > 0 )
  {
    for( auto &s : vm["setup-script"].as<vector<string>>() )
      setup_scripts.push_back(s);
  }

  vector<string> cleanup_scripts;
  if( vm.count("cleanup-script") > 0 )
  {
    for( auto &s : vm["cleanup-script"].as<vector<string>>() )
      cleanup_scripts.push_back(s);
  }






  // create the session that will run the script
  Session session(session_filename,vm["shell"].as<string>());

  // configure the session
  if( vm.count("setup-command") > 0 )
  {
    for( auto &s : vm["setup-command"].as<vector<string>>() )
      session.setup_commands.push_back(s);
  }

  if( vm.count("cleanup-command") > 0 )
  {
    for( auto &s : vm["cleanup-command"].as<vector<string>>() )
      session.cleanup_commands.push_back(s);
  }
  

  
  try {
    for( auto &s : setup_scripts )
      process::system(s.c_str());

    // run the session
    session.run();

    for( auto &s : cleanup_scripts )
      process::system(s.c_str());

  }
  catch(const return_exception& e)
  {
    BOOST_LOG_TRIVIAL(debug) << "Exiting.";
    return 0;
  }
  catch(const std::runtime_error& e)
  {
    BOOST_LOG_TRIVIAL(error) << "An error occurred: " << e.what();
    return 1;
  }
  catch(...)
  {
    BOOST_LOG_TRIVIAL(error) << "An unknown error occurred.";
    return 1;
  }


  return 0;
}
