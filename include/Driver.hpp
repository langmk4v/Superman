#pragma once

#include <vector>

#include "SourceFile.hpp"

namespace fire {
  class Driver {
    std::vector<SourceCode*> inputs;

    std::string cwd; // at start of the program

  public:
    Driver();

    static Driver* get_instance();

    std::string get_first_cwd() const { return cwd; }

    int main(int argc, char** argv);
  };
} // namespace fire