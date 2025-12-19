#include "Strings.hpp"

namespace superman::strings {

  static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;

  std::string to_utf8(std::u16string const& s) { return conv.to_bytes(s); }

  std::u16string to_utf16(std::string const& s) { return conv.from_bytes(s); }

} // namespace superman::strings
