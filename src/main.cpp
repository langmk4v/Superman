#include <iostream>

#include "Driver.hpp"

#include "FSWrap.hpp"

void test() {
  std::cout << fire::FileSystem::GetFolderOfFile("./test/Parser/Node.fire") << std::endl;
  fire::FileSystem::GetDirectory("./test").Dump(0);
}

int main(int argc, char** argv) { return fire::Driver().main(argc, argv); }