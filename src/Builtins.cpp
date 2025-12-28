#include <cstring>
#include <cstdlib>

#include <iostream>
#include <string>

#include "Object.hpp"
#include "BuiltinFunc.hpp"

#include "utf.hpp"

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
// char::is_digit(self) -> bool
//
IMPL(char_is_digit) {
  ObjChar* self = args[0]->as<ObjChar>();
  return new ObjBool(self->val >= u'0' && self->val <= u'9');
}

//
// string::length(self) -> int
//
IMPL(string_length) {
  ObjString* self = args[0]->as<ObjString>();
  return new ObjInt(static_cast<std::int64_t>(self->data.size()));
}

//
// string::starts(self, string) -> bool
//
IMPL(string_starts) {
  ObjString* self = args[0]->as<ObjString>();
  ObjString* prefix = args[1]->as<ObjString>();
  return new ObjBool(std::memcmp(self->data.data(), prefix->data.data(),
                                 prefix->data.size() * sizeof(char16_t)) == 0);
}

//
// string::to_int(self) -> int
//
IMPL(string_to_int) {
  ObjString* self = args[0]->as<ObjString>();
  return new ObjInt(std::stoll(utf::utf16_to_utf8(self->data)));
}

//
// vector::append(self, value) -> vector
//
IMPL(vector_append) {
  ObjVector* self = args[0]->as<ObjVector>();
  self->append(args[1]);
  return self;
}

//
// list::push(self, value) -> list
//
IMPL(list_push) {
  ObjList* self = args[0]->as<ObjList>();
  self->push(args[1]);
  return self;
}

//
// list::push_front(self, value) -> list
//
IMPL(list_push_front) {
  ObjList* self = args[0]->as<ObjList>();
  self->push_front(args[1]);
  return self;
}

//
// list::pop_front(self) -> T
//
IMPL(list_pop_front) {
  ObjList* self = args[0]->as<ObjList>();
  return self->pop_front()->clone();
}

//
// list::pop_back(self) -> T
//
IMPL(list_pop_back) {
  ObjList* self = args[0]->as<ObjList>();
  return self->pop_back()->clone();
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

BuiltinFunc bltm_char_is_digit{
    .name = "is_digit",
    .is_var_args = false,
    .self_type = TypeKind::Char,
    .result_type = TypeKind::Bool,
    .impl = impl_char_is_digit,
};

BuiltinFunc bltm_string_length{
    .name = "length",
    .is_var_args = false,
    .self_type = TypeKind::String,
    .result_type = TypeKind::Int,
    .impl = impl_string_length,
};

BuiltinFunc bltm_string_starts{
    .name = "starts",
    .is_var_args = false,
    .self_type = TypeKind::String,
    .arg_types = {TypeKind::String},
    .result_type = TypeKind::Bool,
    .impl = impl_string_starts,
};

BuiltinFunc bltm_string_to_int{
    .name = "to_int",
    .is_var_args = false,
    .self_type = TypeKind::String,
    .result_type = TypeKind::Int,
    .impl = impl_string_to_int,
};

BuiltinFunc bltm_vector_append{
    .name = "append",
    .is_var_args = false,
    .self_type = TypeKind::Vector,
    .arg_types = {TypeKind::Any},
    .result_type = TypeKind::Vector,
    .returning_self = true,
    .impl = impl_vector_append,
};

BuiltinFunc bltm_list_push{
    .name = "push",
    .is_var_args = false,
    .self_type = TypeKind::List,
    .arg_types = {TypeKind::Any},
    .result_type = TypeKind::List,
    .returning_self = true,
    .impl = impl_list_push,
};

BuiltinFunc bltm_list_pop{
    .name = "pop",
    .is_var_args = false,
    .self_type = TypeKind::List,
    .result_type = TypeKind::Any,
    .impl = impl_list_pop_back,
};

BuiltinFunc bltm_list_push_front{
    .name = "push_front",
    .is_var_args = false,
    .self_type = TypeKind::List,
    .arg_types = {TypeKind::Any},
    .result_type = TypeKind::List,
    .returning_self = true,
    .impl = impl_list_push_front,
};

BuiltinFunc bltm_list_pop_front{
    .name = "pop_front",
    .is_var_args = false,
    .self_type = TypeKind::List,
    .result_type = TypeKind::Any,
    .impl = impl_list_pop_front,
};

} // namespace fire