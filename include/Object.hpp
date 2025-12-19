#pragma once

#include <cstdint>
#include "TypeInfo.hpp"

namespace superman {
  struct ObjNone;
  struct Object {
    TypeInfo type;
    int ref_count = 0;
    virtual ~Object() = default;

    static ObjNone* none;

  protected:
    Object(TypeInfo type) : type(std::move(type)) {}
  };

  struct ObjNone : Object{
    ObjNone():Object(TypeKind::None){}
  };

  struct ObjInt : Object {
    std::int64_t val;
    ObjInt(std::int64_t v) : Object(TypeKind::Int), val(v) {}
  };

  struct ObjFloat : Object {
    double val;
    ObjFloat(double v) : Object(TypeKind::Float), val(v) {}
  };

  struct ObjBool : Object {
    bool val;
    ObjBool(bool v) : Object(TypeKind::Bool), val(v) {}
  };

  struct ObjString : Object {
    std::u16string val;
    ObjString& append(char16_t c);
    ObjString& append(std::u16string const& c);
    ObjString(std::u16string const& s) : Object(TypeKind::String), val(s) {}
  };

  struct ObjChar : Object {
    char16_t val;
    ObjChar(char16_t c) : Object(TypeKind::Char), val(c) {}
    ObjString* to_str() { return new ObjString(std::u16string(1, val)); }
  };
} // namespace superman