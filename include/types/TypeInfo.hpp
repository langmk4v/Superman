#pragma once

#include <vector>

namespace fire {
  struct NdClass;
  struct NdEnum;

  enum class TypeKind {
    None,
    Int,
    Float,
    USize,
    Bool,
    Char,
    String,
    Vector,
    List,
    Tuple,
    Dict,
    Function,
    Class, // => クラス名、インスタンスどちらにも使用
    Enum,
  };

  struct TypeInfo {
    TypeKind kind;
    std::vector<TypeInfo> parameters;

    bool is_ref = false;
    bool is_const = false;

    NdClass* class_node = nullptr;
    NdEnum* enum_node = nullptr;

    TypeInfo(TypeKind k = TypeKind::None) : kind(k) {
    }

    TypeInfo(TypeKind k, std::vector<TypeInfo> v, bool isRef, bool isConst)
        : kind(k), parameters(std::move(v)), is_ref(isRef), is_const(isConst) {
    }

    bool is(TypeKind k) const {
      return kind == k;
    }

    bool is_numeric() const {
      return is(TypeKind::Int) || is(TypeKind::Float);
    }

    bool equals(TypeInfo const& t, bool cmp_ref = true, bool cmp_const = true) const;

    std::string to_string() const;

    static int required_param_count(TypeKind K) {
      if (K == TypeKind::Vector)
        return 1; // <element_type>
      if (K == TypeKind::List)
        return 1; // <element_type>
      if (K == TypeKind::Tuple)
        return -1; // <...>
      if (K == TypeKind::Dict)
        return 2; // <key, value>
      if (K == TypeKind::Function)
        return -1; // <result_type, args...>

      return 0; // Not template!
    }
  };
} // namespace fire