#include "Application.hpp"

namespace superman{
  ObjNone* Object::none = new ObjNone( );
}

int main(int argc, char** argv) {
  return superman::Application().main(argc, argv);
}