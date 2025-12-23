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
    OP_Jmp,
    OP_Jmpz,
    OP_Ret,
    OP_Vardef,
    OP_Label,
  };

  struct Instruction {
    Operations op = OP_Nop;

    parser::Node* expr = nullptr;

    std::string label = "";

    size_t var_index = 0;

    Instruction* addr = nullptr;

    std::string to_string() const;
  };

}