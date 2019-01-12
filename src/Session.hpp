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

struct Session
{
  std::string filename;
  std::string shell;
  std::vector<std::string> shell_args;
  std::vector<std::string> setup_commands;
  std::vector<std::string> cleanup_commands;

  CharTree multi_char_keys;

  std::thread slave_output_thread;
  std::thread monitor_handler_thread;


  SessionScript script;
  SessionState state;
  termios terminal_settings;

  Session(std::string filename, std::string shell = "");
  ~Session();


  int run();
  int get_from_stdin(char& c);
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

  bool isComment(std::string);

  void sync_window_size();

  void shutdown();

  int num_chars_in_next_key();

};




#endif // include protector
