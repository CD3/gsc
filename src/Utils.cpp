#include <string>
#include <regex>
#include "./Utils.hpp"

/**
 * Render a template string using a context
 */
std::string render( std::string templ, const Context& context, std::string stag, std::string etag )
{
  for( auto &c : context )
  {
    std::string pat = stag+c.first+etag;
    std::regex re(pat);
    templ = std::regex_replace( templ, re, c.second);
  }

  return templ;
}
