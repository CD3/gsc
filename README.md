# `gsc` - guided shell scripts

A simple utility that allows you to run guided scripts on Linux, useful for giving command line demonstrations.

**`gsc` has been rewritten**

`gsc` has been completely rewritten from scatch. New features include:

- Modal (insert, command, and pass through mode).
- Support for monitoring sessions via socket connection.
- "simulated typing" has been replaced with actual typing. Just hit a key (in insert mode) to have the next character loaded.
- (More) graceful failure.

## Description

`gsc` creates a pseudo terminal and runs a script by passing each line of the script to the pseudo terminal so that
the script (including command prompts and input) behaves just as it would if the
user had typed the lines themselves.  Each line is sent to the terminal one character at a time. When a full line
has been sent, `gsc` will wait for the user to press `<Enter>` before sending the return character to the terminal.
It is similar to `script`, but you actually generate the terminal output live and incrementally.

This is very useful for giving command line demonstrations. You can load each line of the script into the terminal, discuss the command that is about
to be executed, and then press `<Enter>` to execute the command and see its output.

## Installing

`gsc` uses `cmake` to manage build configurations. To build and install, run
the following commands

```
> git clone https://github.com/CD3/gsc
> cd gsc
> mkdir build
> cd build
> cmake ..
> make
> sudo make install
```

This will compile the `gsc` executable and copy it to the install directory. The
default install directory is `/usr/local/bin`. Use the `-DCMAKE_INSTALL_PREFIX` option
to change this. For example, to install into a sub-directory of your home directory

```
> cmake -DCMAKE_INSTALL_PREFIX=/home/username/install ..
```

## Features

- Run commands saved in a plain text file in a live terminal.
- Scripts are loaded interactively. Go as fast or as slow as you want.
- Script **any** terminal-based programs, including `gnuplot` and `vim`.
- Specify setup and cleanup scripts that should be ran before and after your demo.
- Works transparently with your terminal emulator and shell. The shell's output is written directly to standard output,
  so any escape sequences printed by the shell will be interpreted by your terminal
- Support for simple tag replacement in scripts. For example, the line `mkdir %semester%-%year%` can be rendered to give `mkdir Fall-2019` during the demo.


## Usage

```
Usage: .gsc [OPTIONS] <session-file>
Global options:
  -h [ --help ]                 print help message
  -d [ --debug ]                debug mode. print everything.
  --shell arg                   use shell instead of default.
  --monitor-port arg (=3000)    port to use for monitor socket connections.
  -a [ --auto ]                 run script in auto-pilot without waiting for 
                                user input. useful for testing.
  --auto-pause arg (=100)       number of milliseconds to pause between key 
                                presses in auto-pilot.
  --setup-script arg            may be given multiple times. executables that 
                                will be ran before the session starts.
  --cleanup-script arg          may be given multiple times. executable that 
                                will be ran after the session finishes.
  --setup-command arg           may be given multiple times. command that will 
                                be passed to the session shell before any 
                                script lines.
  --cleanup-command arg         may be given multiple times. command that will 
                                be passed to the session shell before any 
                                script lines.
  -v [ --context-variable ] arg add context variable for string formatting.
  -k [ --key-binding ] arg      add keybinding in k=action format. only integer
                                keycodes are supported. example: 
                                '127:InsertMode_BackOneCharacter' will set 
                                backspace to backup one character in insert 
                                mode (default behavior)
  --list-key-bindings           list all default keybindings.
  --config-file arg             config file to read additional options from.
  --log-file arg                log file name.
  --session-file arg            script file to run.


```

### Input Modes

#### Insert Mode

`gsc` starts up in insert mode. Characters from the script are loaded one at a time as you press any key (so you
can just type as fast as you want without making errors) until the end of the line is reached. `gsc` will then wait for you
to press enter before it sends the return character to the shell, so you don't accidentally run a command before you want to.

#### Command Mode

Pressing Esc in insert mode will switch to command mode. In command mode, you can still load characters from the script one
at a time with the `<Enter>` key, but other keys are used to issue commands to `gsc`. For example, pressing 'q' in command mode
will exit `gsc`.

Currently, the following commands are implemented:

- `i`: switch to insert mode
- `q`: quit
- `p`: switch to pass through mode

#### Pass through Mode

Pressing 'p' in command mode will switfh to pass through mode. In pass through mode, all key presses are passed directly to the shell,
except for Ctrl-D, which is used to exit pass through mode and switch to command mode.



## Motivation

This utility was created out of a need to run live demos of command line utilities in a computing class that I teach. The problem is, I am not an accurate typer. I also
tend to forget things that I want to demonstrate once I get in front of a class. So, I wanted some way to script my demo before class, so that I could focus on explaining
the commands and their output, rather than remembering what to demo.

## History

### Before `gsc`
The idea for this program came during a scientific computing class I teach where students learn how to use Linux and the command line.
The class consisted of many follow along demonstrations, and I wanted to be able to write the demonstration before hand so that I didn't have to type
live in front of the class. I found that when trying to demo tools live, I would often forget to cover some things, and of course make lots of typos.


I couldn't just run shell scripts, because I needed to pause before running each command. So the first incarnation of what eventually became `gsc` was a Perl script
that read lines from a file, waited for the user to hit enter, and then ran each as shell commands. This was exactly what I needed, but it didn't take long for me to want
more. For example, some times I would want to pause the demo, run some commands interactively, and then resume the demo.
Some commands required input that you don't want to store in a file, such as `sudo`. Sometimes I needed to exit the demo early, sometimes I needed to backup and repeat a command.
I realized that what I wanted was the ability to pass the script runner some
basic commands that would allow me to control the demo. On top of all this, the
Perl script only worked with shell commands, I couldn't create a script that
started `gnuplot` and then demo it.

### Modified `ttyrec`

After a lot of banging my head, I found that my problem was rooted deep in the bowls of the Unix terminal. Specifically with how stdin and stdout are handled and their async nature.
Ultimately, I discovered
that the pseudo terminal was the solution. Initially, I tried to use some the Perl pty modules, but I couldn't get them to work (looking back, it was probably because I didn't understand
what was going on). Eventually I stumbled across `ttyrec` and friends and found that it did something very similar to what I needed (so does `script`, which I was unaware of).
I was able to modify the `ttyrec` source to read lines from a file and run them, waiting for the user to hit enter each time, which I called `ttyrun`. I was even able to add some basic
support for commands, so the user could type some command instead of just hitting enter make certain things happen.

### The first `gsc`

Everything was good with `ttyrun`, it pretty much did what I wanted, except that I really had no idea how it worked. I didn't understand the pseudo terminal, and it was getting more
difficult to add features that required me to modify more than just what I had hacked. So, I decided to step back and start over. I finally did some reading and figured out how the
pseudo terminal worked (just barely, I'm still not an expert by any means). `gsc` is the result of me starting with a tutorial on pseudo terminals
(http://rachid.koucha.free.fr/tech_corner/pty_pdip.html) and building a program that I could understand. The original code was pretty much just the example given in the tutorial, but
I have gradually modified/updated it to add more features. The first thing I did was switch to C++. Most of the code that interacts with the pseudo terminal and the user is still straight C, but
I wanted to use C++ features when convenient (for example, reading a file into a vector of strings).

### The rewrite

I used the first version of `gsc` every day for a full semester, and it worked pretty well. There were still a few things I wanted to add. For example, it would be nice to have some way to
display the non-printable characters that are sent to the terminal, so you could see the control commands when demoing programs like `vim`. There were also several interface quirks that
needed to be fixed. When the new semester was getting close, I got motivated enough to work on it again, but found it difficult to make modifications. The code was poorly designed (actually,
I didn't really design anything, I just started hacking on the original tutorial that I got working) and there weren't many unit tests in place since everything was basically implemented as
as functions that worked with global variables. There were also several features I had implemented and not found to be useful in practice.

So, I decided to rewrite `gsc` with following goals:

- Build a properly designed `C++` application.
    - Move code out of main an into classes (functions that access common state are OK as long as they are member functions of a class).
- Use C++17.
- Use the standard library where ever possible.
- Use `boost` for everything else.
- Write unit tests for the interface.

I also took some time to read "The Linux Programming Interface: A Linux and UNIX System Programming Handbook" by Michael Kerrisk to finally figure out pseudo terminals
actually work. I highly recommend this book, it is chock full of information and very easy to read.

Starting with version 0.20, `gsc` has been completely rewritten from scratch. I'm sure it's design could be better, but it's better than it was, and adding features
is easier. If you are using a previous version of `gsc` with a set of demo scripts that you have already developed, you can continue to do so. There are many features
that have been removed from the new version because I either didn't use them, or they are no longer useful with the new interface. For example, `gsc` used to load all script
lines automatically. To make it look like the lines were being typed the user, it would insert a pause after each character was sent. It turns out that pausing for the same
amount of time for each character does not look natural at all, so `gsc` started pausing for a random amount of time (the user could even set limits on the minimum and maximum
amount of time to pause). The new version now waits for a key press before sending characters to the terminal, which allows the user to load lines as fast or slow as they like, and
removes all of the complicated code required to randomly select and pause times.
