#pragma once

#include "Object.hpp"
#include "Builtins.hpp"

namespace fire::vm {
  struct Instruction;
}

namespace fire::vm::interp {

  struct VCPU {
    size_t pc = 0;
    bool result = false;
    Instruction* lr = nullptr;
  };

  class Interp {

    VCPU cpu;
    std::vector<Instruction>& prg;

  public:
    Interp(std::vector<Instruction>& prg);

    Object* eval_expr(parser::Node* node);

    // Object* exe_builtin_func(BuiltinFuncID s, std::vector<Object*>& args);

    void run();

  };

}