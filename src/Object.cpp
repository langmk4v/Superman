#include "Utils.hpp"
#include "Object.hpp"
#include "utf.hpp"

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
    return utf::utf16_to_utf8(std::u16string(1, this->as<ObjChar>()->val));
  }

  case TypeKind::String:
    return utf::utf16_to_utf8(this->as<ObjString>()->data);

  default:
    todoimpl;
  }

  return "none";
}
} // namespace fire