#include "Utils.hpp"
#include "Node.hpp"
#include "TypeInfo.hpp"

namespace fire {

  bool TypeInfo::equals(TypeInfo const& t, bool cr, bool cc) const {

    if (kind != t.kind) return false;

    if (parameters.size() != t.parameters.size()) return false;

    for (auto x = parameters.begin(); auto&& y : t.parameters) {
      if (!y.equals(*x++)) return false;
    }

    if (cr && is_ref != t.is_ref) return false;
    if (cc && is_const != t.is_const) return false;

    return true;
  }

  std::string TypeInfo::to_string() const {
    static char const* names[]{
        "none", "int", "float", "bool", "char", "string", "vector", "tuple", "dict", "function",
    };

    std::string str;

    switch (kind) {
    case TypeKind::Class:
      str = this->class_node->name.text;
      break;

    default:
      str = names[static_cast<size_t>(kind)];
      break;
    }

    if (!parameters.empty()) {
      str += "<";
      for (size_t i = 0; i < parameters.size(); i++) {
        str += parameters[i].to_string();
        if (i + 1 < parameters.size()) str += ", ";
      }
      str += ">";
    }

    if (is_ref) str += " ref";
    if (is_const) str += " const";

    return str;
  }

} // namespace fire