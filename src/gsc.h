#ifndef gsc_h
#define gsc_h

/** @file gsc.h
  * @brief 
  * @author C.D. Clark III
  * @date 12/21/18
  */



#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <exception>
#include <map>
#include <thread>


enum class AutoPilot {ON,OFF};
enum class UserInputMode {COMMAND, INSERT, PASSTHROUGH};
enum class LineStatus {EMPTY, INPROCESS, LOADED, RELOAD};
enum class OutputMode {ALL, NONE, FILTERED};

class return_exception : public std::exception {};
using Context = std::map<std::string, std::string>;

struct SessionState
{
  UserInputMode input_mode = UserInputMode::INSERT;
  LineStatus line_status = LineStatus::EMPTY;
  OutputMode output_mode = OutputMode::ALL;
  AutoPilot auto_pilot = AutoPilot::OFF;

  int masterfd = -2;
  int slavefd = -2;

  char *slave_device_name = NULL;
  pid_t slavePID = -2;
  bool shutdown = false;

  int monitor_port = 3000;
  int monitor_serverfd = -2;

  int auto_pilot_pause_milliseconds = 10;

  termios terminal_settings;
  winsize window_size;

  std::vector<std::string>::iterator script_line_it;
  std::string::iterator line_character_it;
};

struct SessionScript
{
  std::vector<std::string> lines;
  Context context;
  std::string render_stag = "%";
  std::string render_etag = "%";

  void load(const std::string& filename);
  void render();

};


struct Session
{
  std::string filename;
  std::string shell;
  std::vector<std::string> shell_args;
  std::vector<std::string> setup_commands;
  std::vector<std::string> cleanup_commands;

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

};

std::string render( std::string templ, const Context& context, std::string stag="%", std::string etag="%" );



#endif // include protector
