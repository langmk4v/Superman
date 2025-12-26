#pragma once

#include <vector>
#include <cstdint>

#include "Object.hpp"

namespace fire {
  struct BuiltinFunc {
    using FuncPointer = Object* (*)(std::vector<Object*>&);

    char const* name = nullptr;
    bool is_var_args = false;
    TypeInfo self_type = { };
    std::vector<TypeInfo> arg_types = {};
    TypeInfo result_type = {};
    bool returning_self = false;
    FuncPointer impl = nullptr;
  };

  extern BuiltinFunc blt_print;
  extern BuiltinFunc blt_println;

  extern BuiltinFunc bltm_string_starts;
  extern BuiltinFunc bltm_vector_append;

  static constexpr BuiltinFunc const* builtin_func_table[] = {
      &blt_print,
      &blt_println,
  };

  static constexpr BuiltinFunc const* builtin_method_table[] = {
    // string::starts(self, string) -> bool
    &bltm_string_starts,

    // vector::append(self, value)
    &bltm_vector_append,
  };
} // namespace fire