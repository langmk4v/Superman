#pragma once

#include <string>
#include <vector>
#include <cstdint>

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
    OP_StructDef,
  };

  struct StructDef {
    std::string name;
    std::vector<std::string> fields={};
  };

  struct Instruction {
    Operations op = OP_Nop;

    parser::Node* expr = nullptr;

    std::string label = "";

    std::string var_name = "";

    Instruction* addr = nullptr;

    std::string to_string() const;
  };

}