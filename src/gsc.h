#ifndef gsc_h
#define gsc_h

/** @file gsc.h
  * @brief 
  * @author C.D. Clark III
  * @date 12/21/18
  */



#include <termios.h>
#include <exception>
#include <map>


enum class TypingMode {SIMULATE,NONE,USER};
enum class InteractiveMode {ON, OFF};
enum class ReturnMode {AUTO,MANUAL};
enum class UserInputMode {COMMAND, INSERT, PASSTHROUGH};
enum class LineStatus {EMPTY, INPROCESS, LOADED};

class return_exception : public std::exception {};

using Context = std::map<std::string, std::string>;

enum class IOAction { Discard = 1,
                      SendToStdout = 1<<1,
                      SendToSlave = 1<<2,
                      SendTooMaster = 1<<3,
                      ReadFromStdin = 1<<4,
                      ReadFromSlave = 1<<5,
                      ReadFromMaster = 1<<6
                    };


struct SessionState
{
  UserInputMode input_mode = UserInputMode::INSERT;
  LineStatus line_status = LineStatus::EMPTY;
  int masterfd = -2;
  int slavefd = -2;
  char *slave_device_name = NULL;
  pid_t slavePID = -2;
  bool shutdown = false;

  termios terminal_settings;

  std::vector<std::string>::iterator script_line_it;
  std::string::iterator line_character_it;
};

std::string render( std::string templ, const Context& context, std::string stag="%", std::string etag="%" );

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

  void process_user_input();
  void process_script_line();

  void process_slave_output();


  void init_shell_args();

  bool amParent();
  bool amChild();

};



#endif // include protector
