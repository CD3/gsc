#include "catch.hpp"

#include "SessionScript.hpp"

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

#include "CharTree.hpp"

#include "Keybindings.hpp"


using namespace std;

TEST_CASE("Session Script")
{
  SECTION("Check that")
  {
    SessionScript script;
    SECTION("loading")
    {
      SECTION("missing file throws exception")
      {
        CHECK_THROWS( script.load("missing") );
      }
      SECTION("directory throws exception")
      {
        CHECK_THROWS( script.load(".") );
      }
    }
  }

  SECTION("Load Simple File")
  {
    ofstream out("simple-script.sh");
    out << "ls" << endl;
    out << "pwd" << endl;
    out << "who" << endl;
    out.close();

    SessionScript script;


    script.load("simple-script.sh");
    
    CHECK(script.lines.size() == 3);
    CHECK(script.lines[0] == "ls");
    CHECK(script.lines[1] == "pwd");
    CHECK(script.lines[2] == "who");

  }

  SECTION("Load Template File.")
  {
    ofstream out("simple-script.sh");
    out << "%cmd1%" << endl;
    out << "ls" << endl;
    out << "%cmd2%" << endl;
    out << "%cmd1%" << endl;
    out.close();

    SessionScript script;
    script.context["cmd1"] = "pwd";
    script.context["cmd2"] = "who";


    script.load("simple-script.sh");
    
    CHECK(script.lines.size() == 4);
    CHECK(script.lines[0] == "pwd");
    CHECK(script.lines[1] == "ls");
    CHECK(script.lines[2] == "who");
    CHECK(script.lines[3] == "pwd");
  }

  SECTION("Render Script Lines.")
  {

    SessionScript script;
    script.lines.push_back("first");
    script.lines.push_back("%second%");
    script.lines.push_back("%third%");
    script.context["second"] = "2";
    script.context["third"] = "3";


    CHECK(script.lines.size() == 3);
    CHECK(script.lines[0] == "first");
    CHECK(script.lines[1] == "%second%");
    CHECK(script.lines[2] == "%third%");

    script.render();

    CHECK(script.lines.size() == 3);
    CHECK(script.lines[0] == "first");
    CHECK(script.lines[1] == "2");
    CHECK(script.lines[2] == "3");
  }
}


TEST_CASE("CharTree Tests")
{
  CharTree tree;

  SECTION("Adding Strings")
  {
    CHECK( tree.add("string") == 6 );
    CHECK( tree.add("sting") == 3 );

    CHECK( tree.get_children().count('s') == 1 );
    CHECK( tree.get_children().count('t') == 0 );
    CHECK( tree.get_children().at('s').get_children().count('t') == 1 );
    CHECK( tree.get_children().at('s').get_children().at('t').get_children().count('r') == 1 );
    CHECK( tree.get_children().at('s').get_children().at('t').get_children().count('i') == 1 );
  }

  SECTION("Matching")
  {
    CharTree prefixes;
    prefixes.add("pre");
    prefixes.add("re");
    prefixes.add("post");

    REQUIRE( prefixes.match("prefix")  != nullptr );
    REQUIRE( prefixes.match("postfix") != nullptr );
    REQUIRE( prefixes.match("reflex")  != nullptr );
    REQUIRE( prefixes.match("suffix")  == nullptr );

    REQUIRE( prefixes.match("prefix")->depth()  == 3 );
    REQUIRE( prefixes.match("postfix")->depth() == 4 );
    REQUIRE( prefixes.match("reflex")->depth()  == 2 );

  }

}


TEST_CASE("Keybindings")
{
  Keybindings key_bindings;

  InsertModeActions ia;
  CommandModeActions ca;
  PassthroughModeActions pa;

  CHECK( key_bindings.get( 'a', ia ) == 0 );
  CHECK( ia == InsertModeActions::None );
  CHECK( key_bindings.get( '\r', ia ) == 1 );
  CHECK( ia == InsertModeActions::Return );
  CHECK( key_bindings.get( '', ia ) == 1 );
  CHECK( ia == InsertModeActions::BackOneCharacter);
  CHECK( key_bindings.get( '', ia ) == 1 );
  CHECK( ia == InsertModeActions::SwitchToCommandMode);




  CHECK( key_bindings.get( 'a', ca ) == 0 );
  CHECK( ca == CommandModeActions::None );
  CHECK( key_bindings.get( '\r', ca ) == 1 );
  CHECK( ca == CommandModeActions::Return );




}
