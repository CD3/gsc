// this file will process commands from the session file
// and user input. 
// 
// The line that is to be processed will be in a string named commandstr and wi will have
// a stringstream named ss that we can use for parsing.


ss.str( commandstr );
ss.clear();
command = "";
ss >> command;

if( command == "sim" )
  command = "simulate_typing";
if( command == "int" )
  command = "interactive";
if( command == "pass" )
  command = "passthrough";

if( command == "simulate_typing" )
{
  ss >> command;
  if(command == "off")
    simulate_typing = false;
  if(command == "on")
    simulate_typing = true;
}
if( command == "interactive" )
{
  ss >> command;
  if(command == "off")
    interactive = false;
  if(command == "on")
    interactive = true;
}
if( command == "pause" )
{
  int count;
  ss >> count;
  pause(count);
}
if( command == "passthrough" )
{
  input[1] = 0;
  termios ts, tso;
  while( rc = read(0, input, 1) > 0 && input[0] != 4)
  {
    write(masterfd, input, 1 ); 
  }
}
if( command == "stdout" )
{
  ss >> command;
  if(command == "off")
    write( outputPipe[1], "stdout off", sizeof("stdout off") );
  if(command == "on")
    write( outputPipe[1], "stdout on", sizeof("stdout on") );
}
if( command == "pause_min" )
{
  ss >> pause_min;
}
if( command == "pause_max" )
{
  ss >> pause_max;
}
