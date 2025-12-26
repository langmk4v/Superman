#include <cstring>
#include <cstdlib>

#include <iostream>
#include <string>

#include "VM.hpp"

#define IMPL(name) Object* impl_##name(std::vector<Object*>& args)

namespace fire {

  IMPL(print) {
    ObjInt* ret = new ObjInt(0);
    std::string s;
    for (auto&& a : args) {
      s = a->to_string();
      ret->val += s.length() + 1;
      std::cout << s << ' ';
    }
    return ret;
  }

  IMPL(println) {
    ObjInt* x = impl_print(args)->as<ObjInt>();
    std::cout << std::endl;
    x->val++;
    return x;
  }

  //
  // string::starts(self, string) -> bool
  //
  IMPL(string_starts) {
    ObjString* self = args[0]->as<ObjString>();
    ObjString* prefix = args[1]->as<ObjString>();
    return new ObjBool(std::memcmp(self->data.data(), prefix->data.data(), prefix->data.size() * sizeof(char16_t)) == 0);
  }

  //
  // vector::append(self, value) -> vector
  //
  IMPL(vector_append) {
    ObjVector* self = args[0]->as<ObjVector>();
    self->append(args[1]);
    return self;
  }

  BuiltinFunc blt_print{
      .name = "print",
      .is_var_args = true,
      .result_type = TypeKind::Int,
      .impl = impl_print,
  };

  BuiltinFunc blt_println{
      .name = "println",
      .is_var_args = true,
      .result_type = TypeKind::Int,
      .impl = impl_println,
  };

  BuiltinFunc bltm_string_starts{
    .name = "starts",
    .is_var_args = false,
    .self_type = TypeKind::String,
    .arg_types = { TypeKind::String },
    .result_type = TypeKind::Bool,
    .impl = impl_string_starts,
  };

  BuiltinFunc bltm_vector_append{
    .name = "append",
    .is_var_args = false,
    .self_type = TypeKind::Vector,
    .arg_types = { TypeKind::Any },
    .result_type = TypeKind::Vector,
    .returning_self = true,
    .impl = impl_vector_append,
  };

} // namespace fire