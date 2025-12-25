#pragma once

#include <vector>

#include "SourceFile.hpp"

namespace fire {
  class Driver {
    std::vector<SourceCode*> inputs;

  public:
    Driver() {}

    int main(int argc, char** argv);
  };
} // namespace fire