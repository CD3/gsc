#include "./Keybindings.hpp"

Keybindings::Keybindings()
{
  add('\r', InsertModeActions::Return);
  add('', InsertModeActions::SwitchToCommandMode);
  add('', InsertModeActions::BackOneCharacter);
  add(-1,   InsertModeActions::SkipOneCharacter);


  add('i',  CommandModeActions::SwitchToInsertMode);
  add('p',  CommandModeActions::SwitchToPassthroughMode);
  add('q',  CommandModeActions::Quit);
  add('\r', CommandModeActions::Return);
  add('r',  CommandModeActions::ResizeWindow);
  add('j',  CommandModeActions::NextLine);
  add('k',  CommandModeActions::PrevLine);
  add('s',  CommandModeActions::TurnOffStdout);
  add('v',  CommandModeActions::TurnOnStdout);
  add('o',  CommandModeActions::ToggleStdout);

  add('', PassthroughModeActions::SwitchToCommandMode);
}

int Keybindings::add( int k, InsertModeActions a)
{
  int count = InsertMode.count(k);
  InsertMode[k] = a;
  return count;
}
int Keybindings::add( int k, CommandModeActions a)
{
  int count = CommandMode.count(k);
  CommandMode[k] = a;
  return count;
}
int Keybindings::add( int k, PassthroughModeActions a)
{
  int count = PassthroughMode.count(k);
  PassthroughMode[k] = a;
  return count;
}

int Keybindings::get( int k, InsertModeActions& a) const
{
  int count = InsertMode.count(k);
  if(count > 0)
    a = InsertMode.at(k);
  else
    a = InsertModeActions::None;
  return count;
}
int Keybindings::get( int k, CommandModeActions& a) const
{
  int count = CommandMode.count(k);
  if(count > 0)
    a = CommandMode.at(k);
  else
    a = CommandModeActions::None;
  return count;
}
int Keybindings::get( int k, PassthroughModeActions& a) const
{
  int count = PassthroughMode.count(k);
  if(count > 0)
    a = PassthroughMode.at(k);
  else
    a = PassthroughModeActions::None;
  return count;
}

