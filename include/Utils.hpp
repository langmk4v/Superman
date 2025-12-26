#pragma once

#include <string>
#include <vector>

#define _COL_RGB(r, g, b) "\e[38;2;" #r ";" #g ";" #b "m"

#define COL_DEFAULT "\033[0m"
#define COL_BOLD "\033[1m"
#define COL_UNDERLINE "\033[4m"
#define COL_UNBOLD "\033[2m"

#define COL_BLACK "\033[30m"
#define COL_RED "\033[31m"
#define COL_GREEN "\033[32m"
#define COL_LIGHT_GREEN _COL_RGB(80, 255, 0)
#define COL_YELLOW "\033[33m"
#define COL_BLUE "\033[34m"
#define COL_MAGENTA "\033[35m"
#define COL_CYAN "\033[36;5m"
#define COL_GRAY "\e[38;5;235m"
#define COL_WHITE "\033[37m"

#define COL_BK_BLACK "\033[40m"
#define COL_BK_RED "\033[41m"
#define COL_BK_GREEN "\033[42m"
#define COL_BK_YELLOW "\033[43m"
#define COL_BK_BLUE "\033[44m"
#define COL_BK_MAGENTA "\033[45m"
#define COL_BK_CYAN "\033[46;5m"
#define COL_BK_WHITE "\033[47m"

#define todoimpl (fprintf(stderr, "\t#todoimpl at %s:%d\n", __FILE__, __LINE__), exit(22))

#define todo todoimpl

#if _FIRE_DEBUG_

#include <cstdlib>
#include <cassert>
#include <iosfwd>

#define debug(...) __VA_ARGS__

#define alert fprintf(stderr, "\t#alert at %s:%d\n", __FILE__, __LINE__)

#define alertexpr(x)                                                                                                   \
  (fprintf(stderr, "\t#alertexpr %s=", #x), (std::cerr << (x)), fprintf(stderr, " at %s:%d\n", __FILE__, __LINE__))

#define printd(value) (std::cout << COL_BK_BLUE COL_WHITE << (value) << COL_DEFAULT << std::endl)
#define printdf(fmt, ...) printf(COL_RED fmt COL_DEFAULT, __VA_ARGS__)

#else
#define alert ((void)0)
#define debug(...)
#define printd(...)
#define printdf(...)
#endif

namespace fire {
  using std::string;
  using std::string_literals::operator""s;

  struct Node;

  std::string node2s(Node* node);

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

  static inline std::string operator+(std::string_view a, char const* b) {
    return std::string(a) + b;
  }

  static inline std::string operator+(char const* a, std::string_view b) {
    return a + std::string(b);
  }

  static inline std::string operator+(std::string_view a, std::string_view b) {
    return std::string(a) + std::string(b);
  }
} // namespace fire