#! /bin/bash

set -e

root=${1:-$(git rev-parse --show-toplevel)}
shift

cd $root
mkdir -p testing/include/catch
cd testing/include/catch

rm -f catch.hpp catch.hpp.*
wget https://raw.githubusercontent.com/CatchOrg/Catch2/master/single_include/catch.hpp

cd ..

cat << 'EOF'
Add the following to your CMakeLists.txt in the testing directory

```
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/catch)
```

and add Catch as a depedency to your test targets with `target_link_libraries`. i.e.

```
target_link_libraries(tests Catch)
```

EOF
