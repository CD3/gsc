#ifndef CommandParser_hpp
#define CommandParser_hpp

/** @file CommandParser.hpp
  * @brief 
  * @author C.D. Clark III
  * @date 05/23/19
  */


#include <boost/spirit/home/x3.hpp>

namespace {
  namespace sx3 = boost::spirit::x3;
}
struct CommandParser 
{
  sx3::symbols<std::string> commands;
  CommandParser()
  {
    // to add a new command, add a mapping between
    // a command alias and a command name here
    commands.add("COMMENT", "COMMENT");
    commands.add("C", "COMMENT");
    commands.add("RUN", "RUN");
    commands.add("R", "RUN");
    commands.add("EXIT", "EXIT");
    commands.add("X", "EXIT");
    commands.add("SKIP", "SKIP");
    commands.add("RESUME", "RESUME");
    commands.add("PASSTHROUGH", "PASSTHROUGH");
    commands.add("INSERT", "INSERT");
    commands.add("AUTO", "AUTO");
    commands.add("COMMAND", "COMMAND");
    commands.add("PAUSE", "PAUSE");
    commands.add("STDOUT", "STDOUT");
    commands.add("NOSTDOUT", "NOSTDOUT");
    commands.add("INCLUDE", "INCLUDE");
    commands.add("INC", "INCLUDE");
    commands.add("WAIT", "WAIT");
  }

  std::optional< std::pair< std::string, std::string > > parse( std::string line )
  {
    std::string command, argument;

    auto marker = sx3::char_("#");
    auto sep = sx3::lit(":");
    auto arg = sx3::lexeme[(*sx3::char_)];

    auto commf = [&command]( auto& ctx){ command = sx3::_attr(ctx); };
    auto argf =  [&argument](auto& ctx){ argument = sx3::_attr(ctx); };

    auto inline_command_parser = marker >> commands[commf] >> *(sep >> arg[argf]);

    auto it = line.begin();
    bool match = sx3::phrase_parse(it,
                                   line.end(),
                                   inline_command_parser,
                                   sx3::space);

    if( match && it == line.end() )
    {
      return std::make_pair( command, argument );
    }

    return std::nullopt;

  }
  
};



#endif // include protector

