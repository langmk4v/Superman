#pragma once

#include <string>
#include <vector>
#include <locale>
#include <codecvt>

namespace superman::strings {

  static auto& __get_conv() {
    static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;
    return conv;
  }

  std::string to_utf8(std::u16string const& s) { return __get_conv().to_bytes(s); }

  std::u16string to_utf16(std::string const& s) { return __get_conv().from_bytes(s); }

  template <typename ... Args>
  std::string format(std::string const& fmt, Args&&...args){
    static char buffer[0x1000];
    std::snprintf(buffer,std::size(buffer),fmt.c_str(),std::forward<Args>(args)...);
    return buffer;
  }

  template <typename T, typename F>
  std::string join(std::string sep, std::vector<T> const& v, F to_s) {
    std::string s;
    for (size_t i = 0; i < v.size(); i++) {
      s += to_s(v[i]);
      if (i < v.size() - 1) s += sep;
    }
    return s;
  }
} // namespace superman::strings