#include "./SessionScript.hpp"

#include <string>
#include <fstream>
#include <algorithm>
#include <boost/filesystem.hpp>

#include <iostream>

void SessionScript::load(const std::string& filename)
{
  return this->load(filename, this->lines);
}
void SessionScript::load(const std::string& filename, std::vector<std::string>& a_lines)
{
  if(!boost::filesystem::exists(filename) || boost::filesystem::is_directory(filename))
    throw std::runtime_error("No such file "+filename);

  std::ifstream in(filename.c_str());

  std::string line;
  while( std::getline(in,line) )
  {
    // check for 
    auto match = command_parser.parse(line);
    if(match)
    {
      if( match->first == "INCLUDE" )
      {
        this->load( match->second, a_lines );
        continue; // don't add this line to script
      }
    }

    a_lines.push_back(::render(line, this->context,this->render_stag, this->render_etag));
  }
  in.close();
}

void SessionScript::render()
{
  std::transform( this->lines.begin(), this->lines.end(), this->lines.begin(), [this](std::string& s){ return ::render(s,this->context,this->render_stag, this->render_etag); } );
}

