#pragma once

#include "Object.hpp"
#include "VCPU.hpp"
#include "Builtins.hpp"

namespace fire::vm::interp {

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