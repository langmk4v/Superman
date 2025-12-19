#pragma once

#include <string>
#include <vector>

namespace superman {
  enum class TypeKind {
    None,
    Int,
    Float,
    Bool,
    Char,
    String,
    Vector,
    Tuple,
    Dict,
    Function,
  };

  struct TypeInfo {
    TypeKind kind;
    std::vector<TypeInfo> parameters;

    bool is_ref = false;
    bool is_const = false;

    static int required_param_count(TypeKind K) {
      if(K==TypeKind::Vector)return 1;    // <element_type>
      if(K==TypeKind::Tuple)return -1;    // <...>
      if(K==TypeKind::Dict)return 2;      // <key, value>
      if(K==TypeKind::Function)return -1; // <result_type, args...>

      return 0; // Not template!
    }

    TypeInfo(TypeKind k = TypeKind::None) : kind(k) {}
    
    TypeInfo(TypeKind k, std::vector<TypeInfo> v, bool ir, bool ic)
      : kind(k),parameters(std::move(v)),is_ref(ir),is_const(ic)
    {
    }
  };
} // namespace superman