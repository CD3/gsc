#include "./Keybindings.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>

Keybindings::Keybindings()
{
#define add_name(NAME)  \
  InsertModeActionNames.left.insert( boost::bimap<std::string,InsertModeActions>::left_value_type("InsertMode_"#NAME, InsertModeActions::NAME));

  add_name(Return);
  add_name(SwitchToCommandMode);
  add_name(BackOneCharacter);
  add_name(SkipOneCharacter);
  add_name(None);

  // make sure we have added all actions
  assert(InsertModeActionNames.size() == (int)InsertModeActions::None+1);

#undef add_name

#define add_name(NAME)  \
  CommandModeActionNames.left.insert( boost::bimap<std::string,CommandModeActions>::left_value_type("CommandMode_"#NAME, CommandModeActions::NAME));

  add_name(Return);
  add_name(SwitchToInsertMode);
  add_name(SwitchToPassthroughMode);
  add_name(Quit);
  add_name(ResizeWindow);
  add_name(NextLine);
  add_name(PrevLine);
  add_name(TurnOffStdout);
  add_name(TurnOnStdout);
  add_name(ToggleStdout);
  add_name(None);

  assert(CommandModeActionNames.size() == (int)CommandModeActions::None+1);

#undef add_name

#define add_name(NAME)  \
  PassthroughModeActionNames.left.insert( boost::bimap<std::string,PassthroughModeActions>::left_value_type("PassthroughMode_"#NAME, PassthroughModeActions::NAME));

  add_name(SwitchToCommandMode);
  add_name(None);

  assert(PassthroughModeActionNames.size() == (int)PassthroughModeActions::None+1);
#undef add_name

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

int Keybindings::add( int k, const std::string& a)
{
  try{
  if(boost::starts_with( a, "InsertMode_"))
    return add(k, InsertModeActionNames.left.at(a));
  if(boost::starts_with( a, "CommandMode_"))
    return add(k, CommandModeActionNames.left.at(a));
  if(boost::starts_with( a, "PassthroughMode_"))
    return add(k, PassthroughModeActionNames.left.at(a));
  }catch(...){
    std::cerr << "Cannot add keybinding for '"+a+"'. Command not recognized" << std::endl;
  }
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

std::string Keybindings::str(InsertModeActions a) const
{
  return InsertModeActionNames.right.at(a);
}
std::string Keybindings::str(CommandModeActions a) const
{
  return CommandModeActionNames.right.at(a);
}
std::string Keybindings::str(PassthroughModeActions a) const
{
  return PassthroughModeActionNames.right.at(a);
}
