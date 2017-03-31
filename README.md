# `gsc` - guided shell scripts

A simple utility that allows you to run guided scripts, useful for giving command line demonstrations.

## Description

`gsc` runs a script by passing each line of the script to a terminal so
that the output (including command prompts and input) looks just as it would if the user had typed the lines themselves.
Each line is input into the terminal but not executed until the user presses `<Enter>`. 
It is similar to `script`, but you actually generate the terminal output live and incrementally.

This is very useful for giving command line demonstrations. You can load each line of the script into the terminal, discuss the command that is about
to be executed, and then press `<Enter>` to execute the command and see its output.

## Usage

```
Usage: gsc [options] <session-file>
Global options:
  -h [ --help ]                     print help message
  -i [ --interactive ] arg (=1)     disable/enable interactive mode
  -s [ --simulate-typing ] arg (=1) disable/enable simulating typing
  -t [ --test ]                     run script in non-interactive mode and 
                                    check for errors.
  -p [ --pause ] arg (=0)           pause for given number of deciseconds (1/10
                                    second).
  --rand_pause_min arg (=1)         minimum pause time during simulated typing.
  --rand_pause_max arg (=1)         maximum pause time during simulated typing.
  --shell arg                       use shell instead of default.
  -w [ --wait-chars ] arg           list of characters that will cause script 
                                    to stop and wait for user to press enter.
  --setup-command arg               setup command(s) that will be ran before 
                                    the script.
  --cleanup-command arg             cleanup command(s) that will be ran after 
                                    the script.
  --config arg                      config file with options to read.
  --preview                         write script lines to a file before they 
                                    are loaded.
  --messenger arg                   the method to use for messages
  --list-messengers                 list the supported messengers
  --session-file arg                script file to run.


This is a small utility for running guided shell scripts.
Lines from the script are loaded, but will not be executed
until the user presses <Enter>. This is useful if you need to
give a command line demo, but don't want to run the demo 'live'.

Input lines are read from a script file and passed to a pseudoterminal.
Comment lines (beginning with '#' are not passed to the terminal,
but are parsed for control commands (see below).


Control Commands:

	The behavior of `gsc` can be changed on the fly with control commands.
	Control commands are given in comment lines (lines beginning with a '#') of the session file.
	For example, this line will disable interactive mode (commands will be loaded and executed without user input).

	   # interactive off

	Supported Commands:

	interactive (on|off)      Turn interactive mode on/off.
	simulate_typing (on|off)  Turn typing simulation mode on/off.
	pause COUNT               Pause for COUNT tenths of a second ('pause 5' will pause for one half second) after running each command.
	passthrough               Enable passthrough mode. All user input will be passed directly to the terminal until Ctrl-D.
	stdout (on|off)           Turn stdout of the shell process on/off. This allows you to run some commands silently.
	include "script.sh"       Include the contents of script.sh in this script.

	Several short versions of each command are supported.
	int   -> interactive
	sim   -> simulate_typing
	pass  -> passthrough

	Modes:

		`gsc` supports different modes of operation. Modes are not mutually exclusive, more than one mode can be active at one time.

		interactive mode           The user must hit <Enter> to load and execute commands.
		typing simulation mode     Characters are loaded into the command line one at a time with a short pause between each to
		                           simulate typing. This is useful in demos to give your audience time to process the command
		                           you are demonstrating.
		passthrough mode           The script is paused and input is taken from the user. This is useful if you need to enter a password
		                           or want to run a few extra commands in the middle of a script.


Keyboard Commands:

	Various keyboard commands can be given in interactive mode to modify the normal flow of the script:

		b : backup       go back one line in the script.
		s : skip         skip current line in script.
		p : passthrough  enable passthrough mode.
		x : exit         stop the demo and exit. cleanup commands will still be ran.


```

## Motivation

This utility was created out of a need to run live demos of command line utilities in a computing class that I teach. The problem is, I am not an accurate typer. I also
tend to forget things that I want to demonstrate once I get in front of a class. So, I wanted some way to script my demo before class, so that I could focus on explaining
the commands and their output, rather than remembering what to demo.

## History

The idea for this program came during a scientific computing class I teach where students learn how to use Linux and the command line.
The class consisted of many follow along demonstrations, and I wanted to be able to write the demonstration before hand so that I didn't have to type
live in front of the class. I could just focus on the commands and their output.

I couldn't just run shell scripts, because I needed to pause before running each command. So the first incarnation of what eventually became `gsc` was a Perl script
that read lines from a file, waited for the user to hit enter, and then ran each as shell commands. This was exactly what I needed, but it didn't take long for me to want
more. For example, some times I would want to pause the demo, run some commands interactively, and then resume the demo.
Some commands required input that you don't want to store in a file, such as `sudo`. Sometimes I needed to exit the demo early, sometimes I needed to backup and repeat a command.
I realized that what I wanted was the ability to pass the script runner some
basic commands that would allow me to control the demo. On top of all this, the
Perl script only worked with shell commands, I couldn't create a script that
started `gnuplot` and then demoed it.

After a lot of banging my head, I found that my problem was rooted deep in the bowls of the Unix terminal. Specifically with how stdin and stdout are handled and their async nature.
Ultimately, I discovered
that the pseudo terminal was the solution. Initially, I tried to use some the Perl pty modules, but I couldn't get them to work (looking back, it was probably because I didn't understand
what was going on). Eventually I stumbled across `ttyrec` and friends and found that it did something very similar to what I needed (so does `script`, which I was unaware of).
I was able to modify the `ttyrec` source to read lines from a file and run them, waiting for the user to hit enter each time, which I called `ttyrun`. I was even able to add some basic
support for commands, so the user could type some command instead of just hitting enter make certain things happen.

Everything was good with `ttyrun`, it pretty much did what I wanted, except that I really had no idea how it worked. I didn't understand the pseudo terminal, and it was getting more
difficult to add features that required me to modify more than just what I had hacked. So, I decided to step back and start over. I finally did some reading and figured out how the
pseudo terminal worked (just barely, I'm still not an expert by any means). `gsc` is the result of me starting with a tutorial on pseudo terminals
(http://rachid.koucha.free.fr/tech_corner/pty_pdip.html) and building a program that I could understand. The original code was pretty much just the example given in the tutorial, but
I have gradually modified/updated it to add more features. The first thing I did was switch to C++. Most of the code that interacts with the pseudo terminal and the user is still straight C, but
I wanted to use C++ features when convenient (for example, reading a file into a vector of strings).
