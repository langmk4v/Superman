#pragma once

#include <vector>

namespace fire::parser {
  struct Node;
}

namespace fire::vm {

  struct Instruction;
  struct StructDef;

  class Compiler {

    std::vector<Instruction>& out;

    std::vector<StructDef*>& structs;

    size_t label_index = 0;

  public:
    Compiler(std::vector<Instruction>& out, std::vector<StructDef*>& structs)
      : out(out),
        structs(structs)
    {
    }

    void compile(parser::Node* node);

  #if _FIRE_DEBUG_
    void show_all();
  #endif

  private:

    size_t emit(Instruction&& ins);

    std::string get_label();

  };

}