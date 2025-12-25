#include <string>
#include <iostream>

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

} // namespace fire