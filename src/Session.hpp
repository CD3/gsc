#ifndef Session_hpp
#define Session_hpp

/** @file Session.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/11/19
  */

#include <thread>
#include <string>
#include <vector>

#include <termios.h>
#include <netinet/in.h>

#include "./SessionState.hpp"
#include "./SessionScript.hpp"
#include "./CharTree.hpp"
#include "./Keybindings.hpp"




// we may want a way to disable this
// so we will try to isolate it.
#if 1
#include <boost/spirit/home/x3.hpp>

namespace {
  namespace sx3 = boost::spirit::x3;
}
struct CommandParser 
{
  sx3::symbols<std::string> commands;
  CommandParser()
  {
    // to add a new command, add a mapping between
    // a command alias and a command name here
    commands.add("COMMENT", "COMMENT");
    commands.add("C", "COMMENT");
    commands.add("RUN", "RUN");
    commands.add("R", "RUN");
    commands.add("EXIT", "EXIT");
    commands.add("X", "EXIT");
    commands.add("SKIP", "SKIP");
    commands.add("RESUME", "RESUME");
    commands.add("PASSTHROUGH", "PASSTHROUGH");
    commands.add("INSERT", "INSERT");
    commands.add("AUTO", "AUTO");
    commands.add("COMMAND", "COMMAND");
    commands.add("PAUSE", "PAUSE");
  }

  std::optional< std::pair< std::string, std::string > > parse( std::string line )
  {
    std::string command, argument;

    auto marker = sx3::char_("#");
    auto sep = sx3::lit(":");
    auto arg = sx3::lexeme[(*sx3::char_)];

    auto commf = [&command]( auto& ctx){ command = sx3::_attr(ctx); };
    auto argf =  [&argument](auto& ctx){ argument = sx3::_attr(ctx); };

    auto inline_command_parser = marker >> commands[commf] >> *(sep >> arg[argf]);

    auto it = line.begin();
    bool match = sx3::phrase_parse(it,
                                   line.end(),
                                   inline_command_parser,
                                   sx3::space);

    if( match && it == line.end() )
    {
      return std::make_pair( command, argument );
    }

    return std::nullopt;

  }
  
};

#endif



struct Session
{
  std::string filename;
  std::string shell;
  std::vector<std::string> shell_args;
  std::vector<std::string> setup_commands;
  std::vector<std::string> cleanup_commands;

  CharTree multi_char_keys;

  Keybindings key_bindings;

  std::thread slave_output_thread;
  std::thread monitor_handler_thread;


  SessionScript script;
  SessionState state;
  termios terminal_settings;

  CommandParser command_parser;

  Session(std::string filename, std::string shell = "");
  ~Session();


  int run();
  int get_from_stdin(char& c);
  template<size_t N>
  int get_from_stdin(char (&c)[N]);
  int send_to_stdout(char c);
  int get_from_slave(char& c);
  int send_to_slave(char c);

  int send_state_to_monitor(sockaddr_in*);

  void daemon_process_monitor_requests();


  void process_user_input();
  void process_script_line();

  void daemon_process_slave_output();


  void init_shell_args();

  bool amParent();
  bool amChild();

  void sync_window_size();

  void shutdown(bool early = false);

  int num_chars_in_next_key();


};





#endif // include protector
