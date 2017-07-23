#include "catch.hpp"
#include "fakeit.hpp"

#include "gsc.h"



TEST_CASE( "Filename manipulations", "" ) {

  CHECK( gsc::basename("/path/to/file") == "file" );
  CHECK( gsc::dirname("/path/to/file") == "/path/to" );
  CHECK( gsc::path_join("path","to") == "path/to" );
  CHECK( gsc::path_join("","path") == "/path" );

}

TEST_CASE( "String templates", "" ) {

  gsc::Context context;
  CHECK( gsc::fmt("test", context) == "test" );
  context["greeting"] = "Hello";
  CHECK( gsc::fmt("test", context) == "test" );
  CHECK( gsc::fmt("%greeting%", context) == "Hello" );
  CHECK( gsc::fmt("%greeting% World!", context) == "Hello World!" );


}

TEST_CASE( "String manipulations", "" ) {

  CHECK( gsc::ltrim("  test  ") == "test  " );
  CHECK( gsc::rtrim("  test  ") == "  test" );
  CHECK( gsc::trim("  test  ") == "test" );


}

