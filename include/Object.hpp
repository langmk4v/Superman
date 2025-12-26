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
    std::vector<char16_t> data;

    ObjString& append(ObjChar*);
    ObjString& append(ObjString*);
    
    Object* clone() const override { return new ObjString(data); }

    static ObjString* from_char16_ptr_move(char16_t* p) {
      auto x = new ObjString();
      for(;*p;p++)x->data.push_back(*p);
      return x;
    }
    
    ObjString():Object(TypeKind::String){}
    ObjString(std::vector<char16_t> const& s) : Object(TypeKind::String), data(s) {}
    ObjString(ObjChar* ch) : Object(TypeKind::String), data(std::vector<char16_t>(1, ch->val)) {}
  };

  struct ObjVector : Object {
    std::vector<Object*> data;
    Object* clone() const override { return new ObjVector(data); }
    ObjVector(std::vector<Object*> const& v) : Object(TypeKind::Vector), data(v) {}
    ObjVector() : Object(TypeKind::Vector) {}
    ObjVector& append(Object* value) {
      data.push_back(value);
      return *this;
    }
  };

} // namespace fire