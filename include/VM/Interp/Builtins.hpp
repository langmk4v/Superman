#pragma once

#include <vector>
#include <utility>
#include <cstdint>

#include "Sema/TypeInfo.hpp"
#include "VM/Interp/Object.hpp"

namespace fire::vm::interp {
  struct BuiltinFunc {
    using FuncPointer = Object*(*)(std::vector<Object*>&);

    char const* name = nullptr;
    bool is_var_args = false;
    std::vector<sema::TypeInfo> arg_types={};
    sema::TypeInfo result_type={};
    FuncPointer impl = nullptr;
  };

  extern BuiltinFunc blt_print;
  extern BuiltinFunc blt_println;

  static constexpr BuiltinFunc const* builtin_func_table[] = {
    &blt_print,
    &blt_println,
  };
}