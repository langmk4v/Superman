#include "Utils.hpp"
#include "Object.hpp"
#include "strconv.hpp"

namespace fire {
  ObjNone* Object::none = new ObjNone();

  std::string Object::to_string() const {
    switch (type.kind) {
    case TypeKind::None:
      break;

    case TypeKind::Int:
      return std::to_string(as<ObjInt>()->val);

    case TypeKind::Float:
      return std::to_string(as<ObjFloat>()->val);

    case TypeKind::Bool:
      return as<ObjBool>()->val ? "true" : "false";

    case TypeKind::Char: {
      char16_t buf[2]{as<ObjChar>()->val,0};
      return utf16_to_utf8_cpp(buf);
    }

    case TypeKind::String:
      return utf16_to_utf8_len_cpp(as<ObjString>()->data.data(), as<ObjString>()->data.size());

    default:
      todoimpl;
    }

    return "none";
  }
} // namespace fire