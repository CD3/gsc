#ifndef Types_hpp
#define Types_hpp

/** @file Types.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/11/19
  */
#include <map>
#include <exception>
#include <string>

class return_exception : public std::exception {};
using Context = std::map<std::string, std::string>;



std::string render( std::string templ, const Context& context, std::string stag="%", std::string etag="%" );


#endif // include protector
