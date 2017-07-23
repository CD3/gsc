if(iflg && interactive_mode)
{
  nc = read(0, input, BUFSIZ);
  input[nc] = 0;

  commandstr = input;
  commandtoks = boost::split( commandtoks, commandstr, boost::is_any_of(" \t,") );

  #include "process_commands.h"
}
