#ifndef SessionScript_hpp
#define SessionScript_hpp

/** @file SessionScript.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 01/11/19
  */

#include <string>
#include <vector>

#include "./Utils.hpp"
#include "./CommandParser.hpp"

struct SessionScript
{
  std::vector<std::string> lines;
  Context context;
  CommandParser command_parser;
  std::string render_stag = "%";
  std::string render_etag = "%";

  void load(const std::string& filename);
  void load(const std::string& filename, std::vector<std::string>& a_lines);
  void render();

};




#endif // include protector
