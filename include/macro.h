#pragma once

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <string>
#include <vector>
#include <tuple>

#include <iosfwd>
#include <memory>
#include <filesystem>

#include <locale>
#include <codecvt>

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

#if _FIRE_DEBUG_
#include <cstdlib>
#include <cassert>
#include <iostream>

#define alert fprintf(stderr, "\t#alert at %s:%d\n", __FILE__, __LINE__)

#define alertexpr(x)                                                                               \
  (fprintf(stderr, "\t#alertexpr %s=", #x), (std::cerr << (x)),                                    \
   fprintf(stderr, " at %s:%d\n", __FILE__, __LINE__))

#define debug(...) __VA_ARGS__
#else
#define alert ((void)0)
#define debug(...)
#endif
