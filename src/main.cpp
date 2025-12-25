#include "Driver.hpp"

#include "FSWrap.hpp"

void test() { fire::FileSystem::GetDirectory("./test").Dump(0); }

int main(int argc, char** argv) {
  test();
  return 0;

  return fire::Driver().main(argc, argv);
}