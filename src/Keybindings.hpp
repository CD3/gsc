#ifndef Keybindings_hpp
#define Keybindings_hpp

/** @file Keybindings.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/12/19
  */

#include "./Enums.hpp"
#include <map>
#include <iostream>
#include <boost/bimap.hpp>

class Keybindings
{
  protected:
    std::map<int,InsertModeActions> InsertMode;
    std::map<int,CommandModeActions> CommandMode;
    std::map<int,PassthroughModeActions> PassthroughMode;
    std::map<int,AutoModeActions> AutoMode;

    boost::bimap<std::string,InsertModeActions> InsertModeActionNames;
    boost::bimap<std::string,CommandModeActions> CommandModeActionNames;
    boost::bimap<std::string,PassthroughModeActions> PassthroughModeActionNames;
    boost::bimap<std::string,AutoModeActions> AutoModeActionNames;


  public:
    Keybindings();
    int add( int, InsertModeActions );
    int add( int, CommandModeActions );
    int add( int, PassthroughModeActions );
    int add( int, AutoModeActions );

    int add( int, const std::string& );
    
    int get( int, InsertModeActions& ) const;
    int get( int, CommandModeActions& ) const;
    int get( int, PassthroughModeActions& ) const;
    int get( int, AutoModeActions& ) const;

    std::string str(InsertModeActions) const;
    std::string str(CommandModeActions) const;
    std::string str(PassthroughModeActions) const;
    std::string str(AutoModeActions) const;

    friend
    std::ostream& operator<<(std::ostream &out, Keybindings);

};


#endif // include protector
