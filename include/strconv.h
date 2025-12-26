#ifndef STRCONV_H
#define STRCONV_H

#include <stddef.h>
#include <uchar.h>

#ifdef __cplusplus
#include <string>
extern "C" {
#endif

// if out is null, allocate new buffer.

char16_t* utf8_to_utf16(char16_t* out, char const* input);
char* utf16_to_utf8(char* out, char16_t const* input);

char16_t* utf8_to_utf16_with_len(char16_t* out, char const* input, size_t len);
char* utf16_to_utf8_with_len(char* out, char16_t const* input, size_t len);

#ifdef __cplusplus
}

inline std::u16string utf8_to_utf16_cpp(char const* input) {
  char16_t* converted = utf8_to_utf16(nullptr, input);
  std::u16string str = converted;
  delete[] converted;
  return str;
}

inline std::string utf16_to_utf8_cpp(char16_t const* input) {
  char* converted = utf16_to_utf8(nullptr, input);
  std::string str = converted;
  delete[] converted;
  return str;
}

inline std::u16string utf8_to_utf16_len_cpp(char const* input, size_t len) {
  char16_t* converted = utf8_to_utf16_with_len(nullptr, input, len);
  std::u16string str = converted;
  delete[] converted;
  return str;
}

inline std::string utf16_to_utf8_len_cpp(char16_t const* input, size_t len) {
  char* converted = utf16_to_utf8_with_len(nullptr, input, len);
  std::string str = converted;
  delete[] converted;
  return str;
}
#endif

#endif /* STRCONV_H */
