#include <string>

#include "strconv.h"

int main() {

  std::string s1="こんにちは あいうえお abcdef";

  char16_t* s2 = utf8_to_utf16(nullptr,s1.c_str());

  for(;*s2;s2++)
    printf("%04X ", *s2);

  puts("");

}