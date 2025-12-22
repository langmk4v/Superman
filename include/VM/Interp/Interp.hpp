#pragma once

#include "Object.hpp"
#include "VCPU.hpp"
#include "Sys.hpp"

namespace fire::vm::interp {

  class Interp {

    VCPU cpu;
    std::vector<Instruction>& prg;

  public:
    Interp(std::vector<Instruction>& prg);

    Object* eval_expr(parser::Node* node);

    Object* call_sys_func(Sys s, std::vector<Object*>& args);

    void run();

  };

}