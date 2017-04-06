// this file will process commands from the session file
// and user input. 
// 
// The line that is to be processed will be in a string named commandstr.
// The line will be tokenized in a vector named commandtoks (using boost split).
// We will have a sring named tok that we can use,
// and we will also have a stringstream named ss if we want to do our own parsing.


tok = "";
if( commandtoks.size() )
{
  tok = commandtoks[0];
  // remove command from the commandstr
  commandstr.erase(0,tok.size());
  commandstr = trim(commandstr);
}


// support for command shorthands and aliases
if( tok == "sim" )
  tok = "simulate_typing";
if( tok == "int" )
  tok = "interactive";
if( tok == "delay" )
  tok = "pause";
if( tok == "pass" )
  tok = "passthrough";
if( tok == "mess" )
  tok = "message";
if( tok == "key" )
  tok = "keysym";


#define get_arg(n,d) \
  tok = d; \
  if(commandtoks.size()>n) \
    tok = commandtoks[n]; \

// command handlers
if( tok == "simulate_typing" )
{
  get_arg(1,"on");
  if(tok == "off")
    simulate_typing_mode = false;
  if(tok == "on")
    simulate_typing_mode = true;
}
if( tok == "interactive" )
{
  get_arg(1,"on");
  if(tok == "off")
    interactive_mode = false;
  if(tok == "on")
    interactive_mode = true;
}
if( tok == "pause" )
{
  get_arg(1,"10");
  int count = boost::lexical_cast<int>(tok);
  pause(count);
}
if( tok == "passthrough" )
{
  passthrough(masterfd);
}
if( tok == "stdout" )
{
  get_arg(1,"on");
  if(tok == "off")
    write( outputPipe[1], "stdout off", sizeof("stdout off") );
  if(tok == "on")
    write( outputPipe[1], "stdout on", sizeof("stdout on") );
}
if( tok == "message" )
{
  message( commandstr );
}
if( tok == "keysym" )
{
  get_arg(1,"on");
  if(tok == "off")
    keysym_mode = false;
  if(tok == "on")
    keysym_mode = true;
}



// user commands
if( tok == "b" ) // back
{
  if( line_loaded )
  {
    // delete line and repeat.
    // user can give another back command if they want
    // to go to previous line
    char c = 0x7f;
    for(j = 0; j < line.size(); j++)
      write(masterfd, &c, 1);

    i -= 1;
  }
  else
  {
    // backup to previous command
    i -= 2;
  }

  continue;
}

if( tok == "s" ) // skip
{
  if( line_loaded )
  {
    // delete line
    char c = 0x7f;
    for(j = 0; j < line.size(); j++)
      write(masterfd, &c, 1);
  }

  continue;
}

if( tok == "p" ) // passthrough
{
  passthrough(masterfd);
}

if( tok == "x" ) // exit
{
  exit = true;
}
