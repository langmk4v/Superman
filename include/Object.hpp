#pragma once

#include <list>
#include <vector>
#include <cstdint>

#include "TypeInfo.hpp"

namespace fire {

struct ObjNone;
struct ObjInt;
struct ObjFloat;
struct ObjBool;
struct ObjChar;
struct ObjString;
struct ObjVector;
struct ObjList;
struct ObjTuple;
struct ObjDict;
struct ObjOption;
struct ObjFunctor;
struct ObjAny;
struct ObjInstance;
struct ObjEnumerator;

struct Object {
  TypeInfo type;
  int ref_count = 0;

  static ObjNone* none;

  template <typename T> T* as() { return (T*)this; }

  template <typename T> T const* as() const { return (T const*)this; }

  ObjInt* as_int() { return as<ObjInt>(); }
  ObjFloat* as_float() { return as<ObjFloat>(); }
  ObjBool* as_bool() { return as<ObjBool>(); }
  ObjChar* as_char() { return as<ObjChar>(); }
  ObjString* as_string() { return as<ObjString>(); }
  ObjVector* as_vector() { return as<ObjVector>(); }
  ObjList* as_list() { return as<ObjList>(); }
  ObjTuple* as_tuple() { return as<ObjTuple>(); }
  ObjDict* as_dict() { return as<ObjDict>(); }
  ObjOption* as_option() { return as<ObjOption>(); }
  ObjFunctor* as_functor() { return as<ObjFunctor>(); }
  ObjAny* as_any() { return as<ObjAny>(); }
  ObjInstance* as_instance() { return as<ObjInstance>(); }
  ObjEnumerator* as_enumerator() { return as<ObjEnumerator>(); }

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
  std::u16string data;

  ObjString& append(ObjChar*);
  ObjString& append(ObjString*);

  Object* clone() const override { return new ObjString(this->data); }

  ObjString() : Object(TypeKind::String) {}

  ObjString(std::u16string const& s) : Object(TypeKind::String), data(s) {}

  ~ObjString() {}
};

struct ObjVector : Object {
  std::vector<Object*> data;

  Object* clone() const override {
    ObjVector* new_obj = new ObjVector();
    for (auto& item : data) {
      new_obj->data.push_back(item->clone());
    }
    return new_obj;
  }

  ObjVector& append(Object* value) {
    data.push_back(value);
    return *this;
  }

  ObjVector(std::vector<Object*> const& v)
      : Object(TypeKind::Vector), data(v) {}

  ObjVector() : Object(TypeKind::Vector) {}

  ~ObjVector() {
    for (auto& item : data)
      delete item;
  }
};

struct ObjList : Object {
  std::list<Object*> data;

  Object* clone() const override {
    ObjList* new_obj = new ObjList();
    for (auto& item : data) {
      new_obj->data.push_back(item->clone());
    }
    return new_obj;
  }

  ObjList(std::list<Object*> const& l) : Object(TypeKind::List), data(l) {}

  ObjList() : Object(TypeKind::List) {}

  ~ObjList() {
    for (auto item : data)
      delete item;
  }

  ObjList& push(Object* value) {
    data.push_back(value);
    return *this;
  }

  void push_front(Object* value) { data.push_front(value); }

  Object* pop_front() {
    if (data.empty()) return nullptr;
    Object* ret = data.front();
    data.pop_front();
    return ret;
  }

  Object* pop_back() {
    if (data.empty()) return nullptr;
    Object* ret = data.back();
    data.pop_back();
    return ret;
  }
};

} // namespace fire