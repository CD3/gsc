#include "catch.hpp"

#include <iostream>
#include <fstream>
#include <boost/process.hpp>
#include <boost/algorithm/string/predicate.hpp>

using namespace boost::process;
using namespace std;

TEST_CASE("Command line option parsing")
{
  ipstream out,err;
  opstream in;
  char buffer[2048];

  SECTION("--help prints description of options.")
  {

    child c("./gsc --help", std_out > out, std_err > err, std_in < in);

    out.read(buffer,2048);

    CHECK( boost::starts_with(buffer,"Global options:") );

    c.terminate();
  }

  SECTION("No arguments prints usage.")
  {

    child c("./gsc", std_out > out, std_err > err, std_in < in);

    out.read(buffer,2048);

    CHECK( boost::starts_with(buffer,"Usage: ./gsc") );

    c.terminate();
  }


}
