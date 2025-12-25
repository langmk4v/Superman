#pragma once

#include <vector>
#include <cstdint>

#include "Object.hpp"

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
} // namespace fire