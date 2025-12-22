#pragma once

#include <vector>

namespace fire::parser {
  struct Node;
}

namespace fire::vm {

  struct Instruction;

  namespace compiler {

    class Compiler {

      std::vector<Instruction>& out;

    public:
      Compiler(std::vector<Instruction>& out);

      void compile(parser::Node* node);

    private:

    };

  }
}