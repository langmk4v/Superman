#pragma once

#include <cstdint>
#include <string>

namespace fire::parser {
  struct Node;
}

namespace fire::vm {

  enum Operations : uint8_t {
    OP_Nop,
    OP_Do,
    OP_Jmpz,
    OP_Label,
  };

  struct Instruction {
    Operations op;

    parser::Node* expr = nullptr;

    size_t label_index = 0;

    Instruction* addr = nullptr;

    std::string to_string() const;

    Instruction(Operations op) : op(op) { }
  };

}