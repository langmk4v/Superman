#pragma once

#include <vector>

namespace fire::parser {
  struct Node;
}

namespace fire::vm {

  struct Instruction;

  class Compiler {

    std::vector<Instruction>& out;

    size_t label_index = 0;

  public:
    Compiler(std::vector<Instruction>& out);

    void compile(parser::Node* node);

    debug(
      void show_all();
    )

  private:

    size_t emit(Instruction&& ins);

    std::string get_label();

  };

}