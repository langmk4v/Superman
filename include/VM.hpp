#pragma once

#include <vector>
#include <cstdint>

#include "BuiltinFunc.hpp"
#include "IRLow.hpp"
#include "Node.hpp"

namespace fire {
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

    Node* expr = nullptr;

    std::string label = "";

    std::string var_name = "";

    Instruction* addr = nullptr;

    std::string to_string() const;
  };

  struct IRLow;

  class Compiler {

    std::vector<Instruction>& out;

    size_t label_index = 0;

  public:
    Compiler(std::vector<Instruction>& out) : out(out) {
    }

    void compile(IRLow* ir);

    static void compile_full(IRLow* ir);
  };
} // namespace fire