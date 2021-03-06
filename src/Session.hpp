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
#include "./CommandParser.hpp"







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

  Session(std::string filename, std::string shell = "", int monitor_prot = 3000);
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
