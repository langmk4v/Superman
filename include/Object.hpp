#pragma once

#include <cstdint>

#include "TypeInfo.hpp"

namespace fire {

  struct ObjNone;

  struct Object {
    TypeInfo type;
    int ref_count = 0;

    static ObjNone* none;

    template <typename T>
    T* as() {
      return (T*)this;
    }

    template <typename T>
    T const* as() const {
      return (T const*)this;
    }

    std::string to_string() const;

    virtual Object* clone() const = 0;

    virtual ~Object() = default;

  protected:
    Object(TypeInfo type) : type(std::move(type)) {}
  };

  struct ObjNone : Object {
    Object* clone() const override { return new ObjNone; }
    ObjNone() : Object(TypeKind::None) {}
  };

  struct ObjInt : Object {
    std::int64_t val;
    Object* clone() const override { return new ObjInt(val); }
    ObjInt(std::int64_t v) : Object(TypeKind::Int), val(v) {}
  };

  struct ObjFloat : Object {
    double val;
    Object* clone() const override { return new ObjFloat(val); }
    ObjFloat(double v) : Object(TypeKind::Float), val(v) {}
  };

  struct ObjBool : Object {
    bool val;
    Object* clone() const override { return new ObjBool(val); }
    ObjBool(bool v) : Object(TypeKind::Bool), val(v) {}
  };

  struct ObjChar : Object {
    char16_t val;
    Object* clone() const override { return new ObjChar(val); }
    ObjChar(char16_t c) : Object(TypeKind::Char), val(c) {}
  };

  struct ObjString : Object {
    std::u16string val;
    ObjString& append(char16_t c);
    ObjString& append(std::u16string const& c);
    Object* clone() const override { return new ObjString(val); }
    ObjString(std::u16string const& s) : Object(TypeKind::String), val(s) {}
    ObjString(ObjChar* ch) : Object(TypeKind::String), val(std::u16string(1, ch->val)) {}
  };

} // namespace fire