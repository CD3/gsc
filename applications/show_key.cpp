#include <unistd.h>
#include <iostream>
int main(int argc, char *argv[])
{
  for(;;)
  {
    int rc;
    char c;
    rc = read(0,&c,1);
    std::cout << " char: " <<static_cast<char>(c)           << std::endl;
    std::cout << "uchar: " <<static_cast<unsigned char>(c)  << std::endl;
    std::cout << "  int: " <<static_cast<int>(c)            << std::endl;
    std::cout << " uint: " <<static_cast<unsigned int>(c)   << std::endl;
    std::cout << std::endl;

  }

  return 0;
}
