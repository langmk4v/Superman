#pragma once

#include <string>

namespace fire {
class utf {
public:
  static std::string utf16_to_utf8(std::u16string const& s);
  static std::u16string utf8_to_utf16(std::string const& s);
};
} // namespace fire