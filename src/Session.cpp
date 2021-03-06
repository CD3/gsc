#include "./Session.hpp"

#include <iostream>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/process.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <fcntl.h>
#include <signal.h>
#include <sys/poll.h>
#include <sys/wait.h>

Session::Session(std::string filename, std::string shell, int monitor_port)
{
  this->filename = filename;
  this->shell    = shell;
  if (this->shell == "")
    this->shell = getenv("SHELL") == NULL ? "sh" : getenv("SHELL");
  this->init_shell_args();

  this->state.monitor_port = monitor_port;

  // setup list of multi-character keys that we know about
  // and want to send together
  multi_char_keys.add("");     // backspace
  multi_char_keys.add("OA");   // up
  multi_char_keys.add("OB");   // down
  multi_char_keys.add("OC");   // right
  multi_char_keys.add("OD");   // left
  multi_char_keys.add("[2~");  // insert
  multi_char_keys.add("OH");   // home
  multi_char_keys.add("[5~");  // page up
  multi_char_keys.add("[3~");  // delete
  multi_char_keys.add("OF");   // end
  multi_char_keys.add("[6~");  // page down

  int rc;
  rc = tcgetattr(0, &terminal_settings);
  if (rc == -1)
    throw std::runtime_error(
        "Could not retrieve terminal settings on stdin file descriptor.");

  state.terminal_settings = terminal_settings;

  // open a pseudoterminal
  state.masterfd = posix_openpt(O_RDWR);
  // setup the save device
  if (state.masterfd < 0)
    throw std::runtime_error("There was a problem opening pty.");
  BOOST_LOG_TRIVIAL(debug) << "Opened pseudoterminal.";
  BOOST_LOG_TRIVIAL(debug) << "  Master device fd: " << state.masterfd;
  if (grantpt(state.masterfd) != 0)
    throw std::runtime_error("Could not grant access to pty slave.");
  if (unlockpt(state.masterfd) != 0)
    throw std::runtime_error("Could not unlock slave pty device.");
  state.slave_device_name = ptsname(state.masterfd);
  if (state.slave_device_name == NULL)
    throw std::runtime_error("Could not get slave pty device name.");
  BOOST_LOG_TRIVIAL(debug) << "  Slave device name: "
                           << state.slave_device_name;

  state.slavePID = fork();
  if (state.slavePID == -1)
    throw std::runtime_error("Could not fork child process.");

  if (amChild()) {
    // child doesn't need masterfd

    BOOST_LOG_TRIVIAL(debug) << "Closing master fd.";
    close(state.masterfd);
    state.masterfd = -2;

    BOOST_LOG_TRIVIAL(debug) << "Creating new session for child.";
    if (setsid() == -1)
      throw std::runtime_error("Could not start new session.");
    BOOST_LOG_TRIVIAL(debug)
        << "Opening slave device in child to act as controlling terminal.";
    state.slavefd = open(state.slave_device_name, O_RDWR);
    if (state.slavefd == -1)
      throw std::runtime_error("Could not open slave device file.");
    BOOST_LOG_TRIVIAL(debug)
        << "Connecting child stdin, stdou, and stderr to slave device.";
    if (dup2(state.slavefd, 0) != 0)
      throw std::runtime_error("Could not connect child stdin to slave device");
    if (dup2(state.slavefd, 1) != 1)
      throw std::runtime_error(
          "Could not connect child stdout to slave device");
    if (dup2(state.slavefd, 2) != 2)
      throw std::runtime_error(
          "Could not connect child stderr to slave device");
  }

  if (state.slavefd > 0) {
    BOOST_LOG_TRIVIAL(debug) << "Closing slave fd.";
    close(state.slavefd);
    state.slavefd = -2;
  }

  if (amChild()) {
    // launch a shell in the child process
    // need to generate a c-style array of strings
    // to pass arguments to the shell
    auto string2cstr = [](std::string &s) {
      return const_cast<char *>(s.c_str());
    };
    std::vector<char *> argv;
    argv.push_back(string2cstr(this->shell));
    std::transform(this->shell_args.begin(), this->shell_args.end(),
                   std::back_inserter(argv), string2cstr);
    argv.push_back(NULL);  // need to null terminate the array
    BOOST_LOG_TRIVIAL(debug)
        << "Launching shell (" << this->shell << ") in child process with "
        << argv.size() << " element array for argv:";
    for (auto &a : argv) {
      if (a != NULL)
        BOOST_LOG_TRIVIAL(debug) << "  " << a;
      else
        BOOST_LOG_TRIVIAL(debug) << "  NULL";
    }
    execvp(this->shell.c_str(), argv.data());
  }

  if (amParent()) {
    if (state.monitor_port > 0) {
      // setup socket to listen for connections from monitors
      BOOST_LOG_TRIVIAL(debug)
          << "Creating file descriptor for incoming monitor connections";
      state.monitor_serverfd = socket(AF_INET, SOCK_DGRAM, 0);
      BOOST_LOG_TRIVIAL(debug)
          << "  Monitor server fd: " << state.monitor_serverfd;

      sockaddr_in address;
      socklen_t   addrlen = sizeof(address);
      memset(&address, 0, addrlen);  // set memory to zero
      address.sin_family      = AF_INET;
      address.sin_addr.s_addr = INADDR_ANY;
      address.sin_port        = htons(state.monitor_port);

      BOOST_LOG_TRIVIAL(debug)
          << "  Binding monitor server to port " << state.monitor_port;

      if (state.monitor_serverfd < 0)
        throw std::runtime_error("Could not created socket for monitor");
      if (bind(state.monitor_serverfd, (sockaddr *)&address, addrlen) == -1)
        throw std::runtime_error("Could not bind socket for monitor");

      monitor_handler_thread =
          std::thread(&Session::daemon_process_monitor_requests, this);

      BOOST_LOG_TRIVIAL(debug) << "Monitor server ready.";
    } else {
      BOOST_LOG_TRIVIAL(debug) << "Monitor server disabled.";
    }

    // create a thread to process output from the
    // slave device.
    slave_output_thread =
        std::thread(&Session::daemon_process_slave_output, this);

    BOOST_LOG_TRIVIAL(debug) << "Slave output handler ready.";

    // set terminal to raw mode
    cfmakeraw(&(state.terminal_settings));
    tcsetattr(0, TCSANOW, &(state.terminal_settings));

    // set the slave window size to match parents
    sync_window_size();
  }
}

Session::~Session()
{
  // note: only the parent process will run this code.
  // the child proc has been replaced.
  BOOST_LOG_TRIVIAL(debug) << "Session::~Session called";
  // signal the threads to shutdown
  state.shutdown = true;
  // wait for the threads to terminate
  slave_output_thread.join();
  if (state.monitor_port > 0) {
    monitor_handler_thread.join();
    close(state.monitor_serverfd);
  }
  close(state.masterfd);
  tcsetattr(0, TCSANOW, &terminal_settings);

  // kill the child process
  BOOST_LOG_TRIVIAL(debug) << "killing slave process";
  kill(state.slavePID, SIGKILL);
  BOOST_LOG_TRIVIAL(debug) << "waiting for slave process";
  waitpid(state.slavePID, NULL, WNOHANG);

  BOOST_LOG_TRIVIAL(debug) << "Session::~Session finished";
}

int Session::run()
{
  BOOST_LOG_TRIVIAL(debug) << "Beginning session run.";

  this->script.load(this->filename);
  state.script_line_it = this->script.lines.begin();

  if (amParent()) {
    // some vars for processing
    // return codes, input chars, and output chars.
    int  rc;
    char ch;
    int  i, n;

    // run setup commands first
    BOOST_LOG_TRIVIAL(debug) << "Running setup commands";
    for (auto &l : setup_commands) {
      BOOST_LOG_TRIVIAL(debug) << "  setup command: " << l;
      for (auto &c : l) {
        send_to_slave(c);
      }
      send_to_slave('\r');
    }
    // process script and user input
    state.script_line_it = this->script.lines.begin();
    while (state.script_line_it != this->script.lines.end()) {
      // process line for commands, comments, etc.
      process_script_line();
      if (state.skipping) {
        state.script_line_it++;
        continue;
      }

      std::string &line = *state.script_line_it;

      // send line to shell
      state.line_status       = LineStatus::EMPTY;
      state.line_character_it = line.begin();
      while (state.line_status !=
             LineStatus::LOADED) {  // loop until the line has been marked as
                                    // loaded

        while (state.line_character_it !=
               line.end()) {  // loop until the last character has been loaded

          // process user input
          process_user_input();
          if (state.line_status == LineStatus::RELOAD)
            break;  // exit without processing char

          // need to get char(s) to load AFTER user input because
          // the user might change the iterator
          n = 1;
          // check if the next characters are part of a multi-character
          // key that should all be sent at the same time.
          if (state.process_mutli_char_keys) {
            auto ptr = multi_char_keys.match(state.line_character_it,
                                             state.script_line_it->end());
            if (ptr != nullptr) n = ptr->depth();
          }
          for (i = 0; i < n; ++i) {
            ch = *state.line_character_it++;
            send_to_slave(ch);
          }
          state.line_status = LineStatus::INPROCESS;
        }
        if (state.line_status == LineStatus::RELOAD)
          break;  // go back to top and start over

        state.line_status = LineStatus::LOADED;

        // process user input.
        // the user may press backspace, in which case
        // the line status will be reset to processing
        // and we will need to go back to the top.
        process_user_input();
        if (state.line_status == LineStatus::LOADED) send_to_slave('\r');
      }

      if (state.line_status != LineStatus::RELOAD)
        state.script_line_it++;  // don't advance line pointer if we need to
                                 // reload
    }

    // run cleanup commands first
    BOOST_LOG_TRIVIAL(debug) << "Running cleanup commands";
    for (auto &l : cleanup_commands) {
      BOOST_LOG_TRIVIAL(debug) << "  cleanup command: " << l;
      for (auto &c : l) {
        send_to_slave(c);
      }
      send_to_slave('\r');
    }

    if (state.input_mode != UserInputMode::AUTO) do {
        get_from_stdin(ch);
      } while (ch != '\r');
    // inform the user that the session has ended.
    for (auto c : "\r\n\r\nSession Finished. Press Enter.\r\n")
      send_to_stdout(c);
    // wait for user before we quit
    if (state.input_mode != UserInputMode::AUTO) do {
        get_from_stdin(ch);
      } while (ch != '\r');
  }

  BOOST_LOG_TRIVIAL(debug) << "Session run completed.";
  return 0;
}

bool Session::amChild() { return state.slavePID == 0; }

bool Session::amParent() { return state.slavePID > 0; }

int Session::get_from_stdin(char &c) { return read(0, &c, 1); }

template<size_t N>
int Session::get_from_stdin(char (&c)[N])
{
  return read(0, c, N);
}

int Session::send_to_stdout(char c)
{
  if (state.output_mode == OutputMode::ALL) return write(1, &c, 1);
  if (state.output_mode == OutputMode::NONE) return 0;
  // handle output filtering...

  return 0;
}

int Session::get_from_slave(char &c)
{
  // output of slave is read from master input
  return read(state.masterfd, &c, 1);
}

int Session::send_to_slave(char c)
{
  // some translation
  if (c == '\n') c = '\r';

  return write(state.masterfd, &c, 1);
}

void Session::init_shell_args()
{
  if (boost::algorithm::ends_with(this->shell, "bash") ||
      boost::algorithm::ends_with(this->shell, "zsh") ||
      boost::algorithm::ends_with(this->shell, "sh")) {
    shell_args.push_back("-i");
  }
}

void Session::process_user_input()
{
  char c;
  char buffer[12];
  int  count;
  bool cont;

  // this function would probably be
  // better with a goto.
  cont = true;
  while (cont) {
    // by default, we loop once.
    cont = false;

    if (state.input_mode == UserInputMode::COMMAND) {
      // =command mode=
      // read characters from standard in and interpret
      // them as commands. this allows the user to modify
      // state.

      CommandModeActions action;
      while (count = get_from_stdin(
                 buffer))  // read input until we recognize a command
      {
        // FIXME: ignore multi-char keys for now
        if (count > 1) continue;

        c = buffer[0];
        if (c == 3)  // Ctl-C
        {
          // if the user presses Ctl-C, we need to send SIGINT to everybody in
          // our process group
          kill(0, SIGINT);
        }
        if (c == 28)  // Ctl-\, which means quit
        {
          kill(0, SIGQUIT);
        }

        key_bindings.get(c, action);

        if (action == CommandModeActions::Quit) shutdown(true);
        if (action == CommandModeActions::ResizeWindow) sync_window_size();
        if (action == CommandModeActions::SwitchToInsertMode) {
          state.input_mode = UserInputMode::INSERT;
          break;
        }
        if (action == CommandModeActions::SwitchToPassthroughMode) {
          state.input_mode = UserInputMode::PASSTHROUGH;
          break;
        }
        if (action == CommandModeActions::SwitchToAutoMode) {
          state.input_mode = UserInputMode::AUTO;
          break;
        }

        if (action == CommandModeActions::TurnOffStdout) {
          state.output_mode = OutputMode::NONE;
        }
        if (action == CommandModeActions::TurnOnStdout) {
          state.output_mode = OutputMode::ALL;
        }
        if (action == CommandModeActions::ToggleStdout) {
          if (state.output_mode == OutputMode::NONE)
            state.output_mode = OutputMode::ALL;
          else if (state.output_mode == OutputMode::ALL)
            state.output_mode = OutputMode::NONE;
        }

        if (action == CommandModeActions::NextLine) {
          if (state.script_line_it < script.lines.end() - 1)
            state.script_line_it++;
          state.line_status = LineStatus::RELOAD;
          break;
        }
        if (action == CommandModeActions::PrevLine) {
          // if the previous line is a command
          // it will be immediatly re-evaluated. so we need to
          // backup until we read a non-command, or the
          // first line.
          do {
            if (state.script_line_it > script.lines.begin())
              state.script_line_it--;
            else
              break;
          } while (command_parser.parse(*state.script_line_it));
          state.line_status = LineStatus::RELOAD;
          break;
        }

        if (action == CommandModeActions::Return) {
          break;
        }
      }
    }

    if (state.input_mode == UserInputMode::INSERT) {
      // =insert mode=
      //
      // just return to back to the caller when
      // a key is pressed, UNLESS
      //
      // - the user presses Esc. then switch to command mode and start over
      // - a shell line has been loaded. then wait for the user to press Return
      // - the user presses Backspace. then pass Backspace to the shell and back
      // up the line_char_it
      InsertModeActions action;
      for (;;)  // start looping
      {
        count = get_from_stdin(buffer);
        if (count > 1) continue;
        c = buffer[0];

        if (c == 3)  // Ctl-C
        {
          // if the user presses Ctl-C, we need to send SIGINT to everybody in
          // our process group
          kill(0, SIGINT);
        }
        if (c == 28)  // Ctl-\, which means quit
        {
          kill(0, SIGQUIT);
        }

        key_bindings.get(c, action);

        if (action == InsertModeActions::BackOneCharacter) {
          // need to: send backspace to shell, rewind the line character
          // iterator, and start over
          LineStatus init_line_status = state.line_status;
          if (state.line_character_it >
              state.script_line_it->begin()) {  // only delete characters if at
                                                // least one is loaded.
            send_to_slave('');
            state.line_character_it--;
            state.line_status = LineStatus::INPROCESS;
          }
          if (state.line_character_it == state.script_line_it->begin()) {
            state.line_status = LineStatus::EMPTY;
          }
          if (init_line_status != LineStatus::LOADED)
            cont = true;  // if line was loaded, we need to break immediately so
                          // that the input loop will be restarted
          break;
        }
        if (action == InsertModeActions::SwitchToCommandMode) {
          state.input_mode = UserInputMode::COMMAND;
          cont             = true;
          break;
        }

        if (action == InsertModeActions::Return) break;

        if (action == InsertModeActions::Disabled) continue;

        // if a line has been loaded, don't return unless
        // the user presses Enter (which is handled above)
        if (state.line_status == LineStatus::LOADED) continue;

        // return back to caller for any key that isn't handled
        // specifically above.
        break;
      }
    }

    if (state.input_mode == UserInputMode::PASSTHROUGH) {
      // =passthrough mode=
      //
      // pass user input to the shell until
      // they press Ctl-D. then switch to command mode
      // and start over.
      // TODO: if the user enters passthrough mode while
      // a shell line is still being loaded, they probably don't want
      // to finish the line. should probably signal that the rest of
      // the line should be discarded. perhaps we could set the
      // line status to loaded.
      //
      // on the other hand, the user could switch to passthrough
      // mode to fix a typo and then want the rest of the line to
      // finish loading.
      PassthroughModeActions action;

      while (get_from_stdin(c)) {
        key_bindings.get(c, action);
        if (action == PassthroughModeActions::SwitchToCommandMode) {
          state.input_mode = UserInputMode::COMMAND;
          cont             = true;
          break;
        }
        send_to_slave(c);
      }
    }

    if (state.input_mode == UserInputMode::AUTO) {
      fd_set fds;
      FD_ZERO(&fds);
      FD_SET(STDIN_FILENO, &fds);

      timeval timeout;
      timeout.tv_sec  = 0;
      timeout.tv_usec = 0;

      int rc;

      rc = select(1, &fds, NULL, NULL, &timeout);
      if (rc < 0)
        throw std::runtime_error("There was a problem polling stdin fd.");

      // if we are in semi-auto mode and a line has been
      // loaded, then we need to wait for user input
      if (state.auto_pilot_mode == AutoPilotMode::SEMI &&
          state.line_status == LineStatus::LOADED)
        cont = true;

      if (rc > 0) {
        AutoModeActions action;

        get_from_stdin(c);
        key_bindings.get(c, action);

        if (c == 3)  // Ctl-C
        {
          // if the user presses Ctl-C, we need to send SIGINT to everybody in
          // our process group
          kill(0, SIGINT);
        }
        if (c == 28)  // Ctl-\, which means quit
        {
          kill(0, SIGQUIT);
        }

        if (action == AutoModeActions::SwitchToCommandMode) {
          state.input_mode = UserInputMode::COMMAND;
        }

        if (action == AutoModeActions::SwitchToFullAuto) {
          state.auto_pilot_mode = AutoPilotMode::FULL;
        }

        if (action == AutoModeActions::SwitchToSemiAuto) {
          state.auto_pilot_mode = AutoPilotMode::SEMI;
        }

        if (action == AutoModeActions::Return) break;
      }

      std::this_thread::sleep_for(
          std::chrono::milliseconds(state.auto_pilot_pause_milliseconds));
    }
  }

  return;
}

void Session::process_script_line()
{
  while (state.script_line_it != this->script.lines.end()) {
    auto match = command_parser.parse(*state.script_line_it);
    if (match) {
      if (match->first == "COMMENT") {
      }
      if (match->first == "RUN") {
        // WARNING: MAKE SURE YOU KNOW WHO WROTE THE SESSION SCRIPT
        std::string name = boost::replace_all_copy(match->second, " ", "_");
        std::string num  = boost::lexical_cast<std::string>(
            state.script_line_it - script.lines.begin());
        std::string out = num + "-" + name + ".out";
        std::string err = num + "-" + name + ".err";
        boost::process::system(match->second.c_str(),
                               boost::process::std_out > out,
                               boost::process::std_err > err);
      }
      if (match->first == "EXIT") {
        shutdown();
      }

      if (match->first == "SKIP") {
        state.skipping = true;
      }
      if (match->first == "RESUME") {
        state.skipping = false;
      }
      if (match->first == "PASSTHROUGH") {
        state.input_mode = UserInputMode::PASSTHROUGH;
      }
      if (match->first == "INSERT") {
        state.input_mode = UserInputMode::INSERT;
      }
      if (match->first == "AUTO") {
        state.input_mode = UserInputMode::AUTO;
      }
      if (match->first == "COMMAND") {
        state.input_mode = UserInputMode::COMMAND;
      }
      if (match->first == "PAUSE") {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(boost::lexical_cast<int>(match->second)));
      }
      if (match->first == "STDOUT") {
          state.output_mode = OutputMode::ALL;
      }
      if (match->first == "NOSTDOUT") {
          state.output_mode = OutputMode::NONE;
      }
      if (match->first == "WAIT") {
        char ch;
        get_from_stdin(ch);
      }

      state.script_line_it++;
      continue;
    }

    break;
  }
}

void Session::daemon_process_slave_output()
{
  char ch;
  int  rc;
  // use a poll to check for data from the slave
  pollfd polls;
  polls.fd     = state.masterfd;
  polls.events = POLLIN;
  while (!state.shutdown) {
    // check for input from the master.
    // note: we currently need to timeout after
    // some time so that we can check the shutdown switch.
    // perhaps we could block indefinatly if we sent a signal instead?
    while ((rc = poll(&polls, 1, 100))) {
      if (rc < 0)
        throw std::runtime_error("There was a problem polling masterfd.");

      get_from_slave(ch);
      // check if slave output should be printed
      send_to_stdout(ch);
    }
  }

  return;
}

void Session::daemon_process_monitor_requests()
{
  int rc;

  pollfd polls;
  polls.fd     = state.monitor_serverfd;
  polls.events = POLLIN;

  int          n;
  sockaddr_in  address;
  socklen_t    addrlen;
  const size_t REQ_BUF_SIZE = 10;
  char         buffer[REQ_BUF_SIZE];

  while (!state.shutdown) {
    while ((rc = poll(&polls, 1, 100))) {
      if (rc < 0)
        throw std::runtime_error("There was a problem polling monitor socket.");

      n = recvfrom(state.monitor_serverfd, buffer, REQ_BUF_SIZE, 0,
                   (sockaddr *)&address, &addrlen);
      buffer[REQ_BUF_SIZE - 1] = '\0';
      BOOST_LOG_TRIVIAL(debug)
          << "Received " << n << " bytes from monitor: " << buffer;

      send_state_to_monitor(&address);
    }
  }

  return;
}

int Session::send_state_to_monitor(sockaddr_in *address)
{
  boost::property_tree::ptree state_t;
  std::stringstream           state_s;
  std::string                 tmp;

  if (state.input_mode == UserInputMode::INSERT)
    state_t.put("input mode", "I");
  else if (state.input_mode == UserInputMode::COMMAND)
    state_t.put("input mode", "C");
  else if (state.input_mode == UserInputMode::PASSTHROUGH)
    state_t.put("input mode", "P");
  else if (state.input_mode == UserInputMode::AUTO &&
           state.auto_pilot_mode == AutoPilotMode::FULL)
    state_t.put("input mode", "FA");
  else if (state.input_mode == UserInputMode::AUTO)
    state_t.put("input mode", "SA");

  if (script.lines.end() - state.script_line_it > 0)
    state_t.put("current line", *state.script_line_it);
  else
    state_t.put("current line", "None");

  if (state.script_line_it - script.lines.begin() > 0)
    state_t.put("previous line", *(state.script_line_it - 1));
  else
    state_t.put("previous line", "None");

  if (script.lines.end() - state.script_line_it > 1)
    state_t.put("next line", *(state.script_line_it + 1));
  else
    state_t.put("next line", "None");

  if (script.lines.end() - state.script_line_it > 0) {
    tmp = std::string(state.script_line_it->begin(), state.line_character_it);
    state_t.put("current line progress", tmp);
  } else {
    state_t.put("current line progress", "");
  }

  state_t.put("current line number",
              1 + state.script_line_it - script.lines.begin());
  state_t.put("total number lines", script.lines.size());

  write_json(state_s, state_t);

  sendto(state.monitor_serverfd, state_s.str().c_str(), state_s.str().size(), 0,
         (sockaddr *)address, sizeof(sockaddr_in));

  return 0;
}

void Session::sync_window_size()
{
  if (ioctl(0, TIOCGWINSZ, &state.window_size) == -1)
    throw std::runtime_error("Could not get current window size");
  if (ioctl(state.masterfd, TIOCSWINSZ, &state.window_size) == -1)
    throw std::runtime_error("Could not set psuedo-terminal window size");
}

void Session::shutdown(bool early)
{
  BOOST_LOG_TRIVIAL(debug) << "Shutdown called.";
  state.shutdown = true;
  if (early)
    throw early_exit_exception();
  else
    throw normal_exit_exception();
}

int Session::num_chars_in_next_key()
{
  int n = 1;

  return n;
}
