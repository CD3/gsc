#ifndef Keybindings_hpp
#define Keybindings_hpp

/** @file Keybindings.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/12/19
  */

#include "./Enums.hpp"
#include <map>

class Keybindings
{
  protected:
    std::map<int,InsertModeActions> InsertMode;
    std::map<int,CommandModeActions> CommandMode;
    std::map<int,PassthroughModeActions> PassthroughMode;

  public:
    Keybindings();
    int add( int, InsertModeActions );
    int add( int, CommandModeActions );
    int add( int, PassthroughModeActions );
    
    int get( int, InsertModeActions& ) const;
    int get( int, CommandModeActions& ) const;
    int get( int, PassthroughModeActions& ) const;

};


#endif // include protector
