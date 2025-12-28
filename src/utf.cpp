#include <string>
#include "utf.hpp"

extern "C" {
char* utf16_to_utf8_c(const char16_t* s, unsigned long len);
char16_t* utf8_to_utf16_c(const char* s, unsigned long len);
void utf_free(void* p);
}

namespace fire {

std::string utf::utf16_to_utf8(std::u16string const& s) {
  char* p = utf16_to_utf8_c(s.data(), static_cast<unsigned long>(s.size()));
  std::string r(p);
  utf_free(p);
  return r;
}

std::u16string utf::utf8_to_utf16(std::string const& s) {
  char16_t* p = utf8_to_utf16_c(s.data(), static_cast<unsigned long>(s.size()));
  std::u16string r(p);
  utf_free(p);
  return r;
}

} // namespace fire