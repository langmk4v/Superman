#pragma once

#include <vector>
#include "IR.hpp"

namespace fire {

  struct BuiltinFunc {
    using FuncPointer = Object* (*)(std::vector<Object*>&);

    char const* name = nullptr;
    bool is_var_args = false;
    std::vector<TypeInfo> arg_types = {};
    TypeInfo result_type = {};
    FuncPointer impl = nullptr;
  };

  extern BuiltinFunc blt_print;
  extern BuiltinFunc blt_println;

  static constexpr BuiltinFunc const* builtin_func_table[] = {
      &blt_print,
      &blt_println,
  };

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

  class Compiler {

    std::vector<Instruction>& out;

    size_t label_index = 0;

  public:
    Compiler(std::vector<Instruction>& out) : out(out) {}

    void compile(IR* ir);

    static void compile_full(IR* ir);
  };
} // namespace fire