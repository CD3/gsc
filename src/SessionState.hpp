#ifndef SessionState_hpp
#define SessionState_hpp

/** @file SessionState.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/11/19
  */

#include <vector>
#include <string>
#include <atomic>

//#include <termios.h>
#include <sys/ioctl.h>

#include "./Enums.hpp"

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
  std::atomic<bool> shutdown;

  int monitor_port = 3000;
  int monitor_serverfd = -2;

  int auto_pilot_pause_milliseconds = 10;

  bool process_mutli_char_keys = true;

  termios terminal_settings;
  winsize window_size;

  std::vector<std::string>::iterator script_line_it;
  std::string::iterator line_character_it;

  SessionState():shutdown(false){}
};



#endif // include protector
