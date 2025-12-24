#pragma once

#include <string>
#include <vector>
#include <locale>
#include <codecvt>

namespace fire {
  using std::string;
  using std::string_literals::operator""s;

  namespace parser {
    struct Node;
  }

  namespace strings {
    std::string to_utf8(std::u16string const& s);
    std::u16string to_utf16(std::string const& s);

    std::string node2s(parser::Node* node);

    template <typename... Args>
    std::string format(std::string const& fmt, Args&&... args) {
      static char buffer[0x1000];
      std::snprintf(buffer, std::size(buffer), fmt.c_str(), std::forward<Args>(args)...);
      return buffer;
    }

    template <typename T, typename F>
    std::string join(std::string const& sep, std::vector<T> const& v, F to_s) {
      std::string s;
      for (size_t i = 0; i < v.size(); i++) {
        s += to_s(v[i]);
        if (i < v.size() - 1) s += sep;
      }
      return s;
    }

    static inline std::string join(std::string const& sep, std::vector<std::string> const& v) {
      std::string s;
      for (size_t i = 0; i < v.size(); i++) {
        s += v[i];
        if (i < v.size() - 1) s += sep;
      }
      return s;
    }
  }
}