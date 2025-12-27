#pragma once

#include <vector>
#include <cstdint>

#include "Object.hpp"

namespace fire {
struct BuiltinFunc {
  using FuncPointer = Object* (*)(std::vector<Object*>&);

  char const* name = nullptr;
  bool is_var_args = false;
  TypeInfo self_type = {};
  std::vector<TypeInfo> arg_types = {};
  TypeInfo result_type = {};
  bool returning_self = false;
  FuncPointer impl = nullptr;
};

extern BuiltinFunc blt_print;
extern BuiltinFunc blt_println;

extern BuiltinFunc bltm_char_is_digit;
extern BuiltinFunc bltm_string_length;
extern BuiltinFunc bltm_string_starts;
extern BuiltinFunc bltm_string_to_int;
extern BuiltinFunc bltm_vector_append;
extern BuiltinFunc bltm_list_push;
extern BuiltinFunc bltm_list_pop;
extern BuiltinFunc bltm_list_push_front;
extern BuiltinFunc bltm_list_pop_front;

static constexpr BuiltinFunc const* builtin_func_table[] = {
    &blt_print,
    &blt_println,
};

static constexpr BuiltinFunc const* builtin_method_table[] = {
    // char::is_digit(self) -> bool
    &bltm_char_is_digit,

    // string::length(self) -> int
    &bltm_string_length,

    // string::starts(self, string) -> bool
    &bltm_string_starts,

    // string::to_int(self) -> int
    &bltm_string_to_int,

    // vector<T>::append(self, value)
    &bltm_vector_append,

    // list<T>::push(self, value)
    &bltm_list_push,

    // list<T>::pop(self) -> T
    &bltm_list_pop,

    // list<T>::push_front(self, value)
    &bltm_list_push_front,

    // list<T>::pop_front(self) -> T
    &bltm_list_pop_front,

};
} // namespace fire