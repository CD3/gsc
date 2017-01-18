mkdir example
cd example
vim main.cpp
# # pause for a second to give vim time to start
# pause 10

i#include <iostream>

int main()
{
  std::cout << "Hello World" << std::endl;
  return 0;
}

:wq
# # pause for a second to give vim time to shutdown
# pause 10
g++ main.cpp -o main
./main
# pause 30
