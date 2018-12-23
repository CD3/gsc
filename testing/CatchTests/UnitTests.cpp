#include "catch.hpp"

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

#include "gsc.h"

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



