#include "./Keybindings.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <iostream>
#include <iomanip>

Keybindings::Keybindings()
{
#define add_name(NAME)  \
  InsertModeActionNames.left.insert( boost::bimap<std::string,InsertModeActions>::left_value_type("InsertMode_"#NAME, InsertModeActions::NAME));

  add_name(Return);
  add_name(SwitchToCommandMode);
  add_name(BackOneCharacter);
  add_name(SkipOneCharacter);
  add_name(Disabled);
  add_name(None);

  // make sure we have added all actions
  assert(InsertModeActionNames.size() == (int)InsertModeActions::None+1);

#undef add_name

#define add_name(NAME)  \
  CommandModeActionNames.left.insert( boost::bimap<std::string,CommandModeActions>::left_value_type("CommandMode_"#NAME, CommandModeActions::NAME));

  add_name(Return);
  add_name(SwitchToInsertMode);
  add_name(SwitchToPassthroughMode);
  add_name(SwitchToAutoMode);
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

#define add_name(NAME)  \
  AutoModeActionNames.left.insert( boost::bimap<std::string,AutoModeActions>::left_value_type("AutoMode_"#NAME, AutoModeActions::NAME));

  add_name(SwitchToCommandMode);
  add_name(SwitchToFullAuto);
  add_name(SwitchToSemiAuto);
  add_name(Return);
  add_name(None);

  assert(AutoModeActionNames.size() == (int)AutoModeActions::None+1);
#undef add_name





  add('\r', InsertModeActions::Return);
  add('', InsertModeActions::SwitchToCommandMode);
  add('', InsertModeActions::BackOneCharacter);
  add('', InsertModeActions::BackOneCharacter);
  add(-1,   InsertModeActions::SkipOneCharacter);


  add('i',  CommandModeActions::SwitchToInsertMode);
  add('p',  CommandModeActions::SwitchToPassthroughMode);
  add('a',  CommandModeActions::SwitchToAutoMode);
  add('q',  CommandModeActions::Quit);
  add('\r', CommandModeActions::Return);
  add('r',  CommandModeActions::ResizeWindow);
  add('j',  CommandModeActions::NextLine);
  add('k',  CommandModeActions::PrevLine);
  add('s',  CommandModeActions::TurnOffStdout);
  add('v',  CommandModeActions::TurnOnStdout);
  add('o',  CommandModeActions::ToggleStdout);

  add('', PassthroughModeActions::SwitchToCommandMode);

  add('', AutoModeActions::SwitchToCommandMode);
  add('f',  AutoModeActions::SwitchToFullAuto);
  add('s',  AutoModeActions::SwitchToSemiAuto);
  add('\r', AutoModeActions::Return);

}

#define MakeAdd( TYPE ) \
int Keybindings::add( int k, TYPE##Actions a)  \
{ \
  int count = TYPE.count(k); \
  TYPE[k] = a; \
  return count; \
}

MakeAdd( InsertMode );
MakeAdd( CommandMode );
MakeAdd( PassthroughMode );
MakeAdd( AutoMode );

#undef MakeAdd


int Keybindings::add( int k, const std::string& a)
{
  try{
  if(boost::starts_with( a, "InsertMode_"))
    return add(k, InsertModeActionNames.left.at(a));
  if(boost::starts_with( a, "CommandMode_"))
    return add(k, CommandModeActionNames.left.at(a));
  if(boost::starts_with( a, "PassthroughMode_"))
    return add(k, PassthroughModeActionNames.left.at(a));
  if(boost::starts_with( a, "AutoMode_"))
    return add(k, AutoModeActionNames.left.at(a));
  }catch(...){
    std::cerr << "Cannot add keybinding for '"+a+"'. Command not recognized" << std::endl;
  }

  return -1;
}

#define MakeGet( TYPE ) \
int Keybindings::get( int k, TYPE##Actions& a) const \
{ \
  int count = TYPE.count(k); \
  if(count > 0) \
    a = TYPE.at(k); \
  else \
    a = TYPE##Actions::None; \
  return count; \
}

MakeGet( InsertMode );
MakeGet( CommandMode );
MakeGet( PassthroughMode );
MakeGet( AutoMode );

#undef MakeGet

#define MakeStr( TYPE ) \
std::string Keybindings::str(TYPE##Actions a) const \
{ \
  return TYPE##ActionNames.right.at(a); \
}

MakeStr( InsertMode );
MakeStr( CommandMode );
MakeStr( PassthroughMode );
MakeStr( AutoMode );

#undef MakeStr


std::ostream& operator<<(std::ostream &out, Keybindings kb)
{

  out << std::left << std::setw(5) << "Key"
      << std::left << std::setw(50) << "Action"
      << "\n";
  out << std::left << std::setw(55) << std::setfill('=') << ""
      << "\n" << std::setfill(' ');
  out << "\n";
  for( auto &e : kb.InsertMode )
  {
    out << std::left  << std::setw(5) << e.first
        << std::left  << std::setw(50) << kb.InsertModeActionNames.right.at(e.second)
        << "\n";
  }

  out << "\n";
  for( auto &e : kb.CommandMode )
  {
    out << std::left  << std::setw(5) << e.first
        << std::left  << std::setw(50) << kb.CommandModeActionNames.right.at(e.second)
        << "\n";
  }

  out << "\n";
  for( auto &e : kb.PassthroughMode )
  {
    out << std::left  << std::setw(5) << e.first
        << std::left  << std::setw(50) << kb.PassthroughModeActionNames.right.at(e.second)
        << "\n";
  }

  out << "\n";
  for( auto &e : kb.AutoMode )
  {
    out << std::left  << std::setw(5) << e.first
        << std::left  << std::setw(50) << kb.AutoModeActionNames.right.at(e.second)
        << "\n";
  }

  return out;
}

