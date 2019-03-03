#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <boost/program_options.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/spirit/home/x3.hpp>

#include "Session.hpp"
#include "Keybindings.hpp"

namespace po = boost::program_options;
using namespace boost;
using namespace std;
using namespace boost::spirit;

std::string manual_text = R"(

)";

void signal_handler(int sig)
{
  BOOST_LOG_TRIVIAL(debug) << "Received signal: " << sig;

  BOOST_LOG_TRIVIAL(debug) << "Throwing early_exit_exception";
  throw early_exit_exception();
}

int main(int argc, char *argv[])
{

  // process command line arguments
  po::options_description options("Global options");
  options.add_options()
    ("help,h"            , "print help message")
    ("debug,d"           , "debug mode. print everything.")
    ("shell"             , po::value<string>()->default_value(""), "use shell instead of default.")
    ("monitor-port"      , po::value<int>()->default_value(3000), "port to use for monitor socket connections.")
    ("auto,a"            , "run script in auto-pilot without waiting for user input. useful for testing.")
    ("auto-pause"        , po::value<int>()->default_value(100), "number of milliseconds to pause between key presses in auto-pilot.")
    ("setup-script"      , po::value<vector<string>>()->composing(), "may be given multiple times. executables that will be ran before the session starts.")
    ("cleanup-script"    , po::value<vector<string>>()->composing(), "may be given multiple times. executable that will be ran after the session finishes.")
    ("setup-command"     , po::value<vector<string>>()->composing(), "may be given multiple times. command that will be passed to the session shell before any script lines.")
    ("cleanup-command"   , po::value<vector<string>>()->composing(), "may be given multiple times. command that will be passed to the session shell before any script lines.")
    ("context-variable,v", po::value<vector<string>>()->composing(), "add context variable for string formatting.")
    ("key-binding,k"     , po::value<vector<string>>()->composing(), "add keybinding in k=action format. only integer keycodes are supported. example: '127:InsertMode_BackOneCharacter' will set backspace to backup one character in insert mode (default behavior)")
    ("list-key-bindings"  , "list all default keybindings.")
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

  if(vm.count("list-key-bindings"))
  {
    Keybindings key_bindings;
    std::cout << key_bindings << std::endl;
    exit(0);
  }

  if(vm.count("session-file") == 0)
  {
    cout << "Usage: " << argv[0] << " [OPTIONS] <session-file>" << endl;
    cout << options << endl;
    exit(0);
  }


  string session_filename = vm["session-file"].as<string>();


  if( !filesystem::exists(session_filename) )
  {
    std::cerr << "No such file '"<<session_filename<<"'"<<std::endl;
    exit(1);
  }


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

  Context c;
  if( vm.count("context-variable") > 0 )
  {

    for( auto &s : vm["context-variable"].as<vector<string>>() )
    {
      string key;
      string val;

      auto keyf = [&](auto& ctx){ key = _attr(ctx);};
      auto valf = [&](auto& ctx){ val = _attr(ctx);};

      
      bool r = x3::phrase_parse( s.begin(), s.end(), x3::lexeme[ "'" >> (+(x3::char_-"'"))[keyf] >> "'"]
                                                  >> "="
                                                  >> x3::lexeme[ "'" >> (+(x3::char_-"'"))[valf] >> "'"]
                                                   , x3::ascii::space );

      c[key] = val;
    }

  }

  // read additional configuration from file(s)
  {
    vector<string> files;
    if( vm.count("config-file") )
    {
      for( auto &f : vm["config-file"].as<vector<string>>() )
        files.push_back(f);
    }
    files.push_back(".gscrc");
    files.push_back("%HOME%/.gscrc");
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






  // create and configure the session that will run the script
  if( std::signal(SIGINT ,signal_handler) == SIG_ERR 
   || std::signal(SIGQUIT,signal_handler) == SIG_ERR )
  {
    throw std::runtime_error("Could not setup signal handler");
  }

  Session session(session_filename,vm["shell"].as<string>());
  session.state.monitor_port = vm["monitor-port"].as<int>();
  if( vm.count("auto") > 0 )
  {
    BOOST_LOG_TRIVIAL(debug) << "Running in full auto mode";
    session.state.input_mode = UserInputMode::AUTO;
    session.state.auto_pilot_mode  = AutoPilotMode::FULL;
    session.state.auto_pilot_pause_milliseconds = vm["auto-pause"].as<int>();
  }
  session.script.context = c;
  session.script.render();

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

  if( vm.count("key-binding") > 0 )
  {
    for( auto s : vm["key-binding"].as<vector<string>>() )
    {
      auto res = boost::find_last(s,":");
      string key( s.begin(), res.begin() );
      string val( res.begin()+1,s.end() );
      boost::trim(key);

      int k;
      try {
        k = boost::lexical_cast<int>(key);
      }catch(const boost::bad_lexical_cast& ){
        std::cerr << "Could not convert keybinding key '"<<key<<"' to int.\r\n";
        std::cerr << "Key-bindings must specify the integer value emitted when a key is pressed.\r\n";
        std::cerr << "Binding to keys that emit more than one integer value is not currently supported.\r"<<std::endl;
      }
      session.key_bindings.add(k,val);
    }
  }




  try {
    for( auto &s : setup_scripts )
      process::system(s.c_str());

    // run the session
    session.run();

    for( auto &s : cleanup_scripts )
      process::system(s.c_str());

  }
  catch(const normal_exit_exception& e)
  {
    BOOST_LOG_TRIVIAL(debug) << "Exiting normally.";
    return 0;
  }
  catch(const early_exit_exception& e)
  {
    BOOST_LOG_TRIVIAL(debug) << "Exiting early.";
    return 1;
  }
  catch(const std::runtime_error& e)
  {
    BOOST_LOG_TRIVIAL(error) << "A runtime error occurred: " << e.what();
    return 2;
  }
  catch(...)
  {
    BOOST_LOG_TRIVIAL(error) << "An unknown error occurred.";
    return 3;
  }


  return 0;
}
