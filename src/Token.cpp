#ifdef _FIRE_DEBUG_
#include <set>
#endif

#include <stdexcept>
#include <cstring>
#include <cstddef>

#include "Token.hpp"

namespace fire {
  struct punct_str_map {
    TokenPunctuators punct;
    char const* str;
  };

  // clang-format off
  static punct_str_map table[] = {
    //
    // この配列内の要素は文字列の長さ順で定義すること
    //
    
    // -- 3 chars --
    { TokenPunctuators::Punct_LShiftAssign, "<<=" },
    { TokenPunctuators::Punct_RShiftAssign, ">>=" },
    
    // -- 2 chars --
    { TokenPunctuators::Punct_AddAssign, "+=" },
    { TokenPunctuators::Punct_SubAssign, "-=" },
    { TokenPunctuators::Punct_MulAssign, "*=" },
    { TokenPunctuators::Punct_DivAssign, "/=" },
    { TokenPunctuators::Punct_ModAssign, "%=" },
    { TokenPunctuators::Punct_AndAssign, "&=" },
    { TokenPunctuators::Punct_OrAssign, "|=" },
    { TokenPunctuators::Punct_XorAssign, "^=" },
    { TokenPunctuators::Punct_Equal, "==" },
    { TokenPunctuators::Punct_NotEqual, "!=" },
    { TokenPunctuators::Punct_GreaterOrEqual, ">=" },
    { TokenPunctuators::Punct_LessOrEqual, "<=" },
    { TokenPunctuators::Punct_And, "&&" },
    { TokenPunctuators::Punct_Or, "||" },
    { TokenPunctuators::Punct_LShift, "<<" },
    { TokenPunctuators::Punct_RShift, ">>" },

    // -- 1 char --
    { TokenPunctuators::Punct_Assign, "=" },
    { TokenPunctuators::Punct_Greater, ">" },
    { TokenPunctuators::Punct_Less, "<" },
    { TokenPunctuators::Punct_Not, "!" },
    { TokenPunctuators::Punct_Add, "+" },
    { TokenPunctuators::Punct_Sub, "-" },
    { TokenPunctuators::Punct_Mul, "*" },
    { TokenPunctuators::Punct_Div, "/" },
    { TokenPunctuators::Punct_Mod, "%" },
    { TokenPunctuators::Punct_BitAnd, "&" },
    { TokenPunctuators::Punct_BitOr, "|" },
    { TokenPunctuators::Punct_BitXor, "^" },
    { TokenPunctuators::Punct_Comma, "," },
    { TokenPunctuators::Punct_Semicolon, ";" },
    { TokenPunctuators::Punct_Colon, ":" },
    { TokenPunctuators::Punct_Dot, "." },
    { TokenPunctuators::Punct_Question, "?" },
    { TokenPunctuators::Punct_ParenOpen, "(" },
    { TokenPunctuators::Punct_ParenClose, ")" },
    { TokenPunctuators::Punct_BracketOpen, "[" },
    { TokenPunctuators::Punct_BracketClose, "]" },
    { TokenPunctuators::Punct_CurlyOpen, "{" },
    { TokenPunctuators::Punct_CurlyClose, "}" },
    { TokenPunctuators::Punct_Hash, "#" },
    { TokenPunctuators::Punct_Dollar, "$" },
    { TokenPunctuators::Punct_Backtick, "`" },
    { TokenPunctuators::Punct_Tilde, "~" },
  };
  // clang-format on

  TokenPunctuators Token::get_punct(char const* s) {
    for (auto const& [punct, str] : table) {
      if (std::strncmp(s, str, std::strlen(s)) == 0) { return punct; }
    }
    return TokenPunctuators::Punct_None;
  }

#ifdef _FIRE_DEBUG_
  void check_duplications() {
    std::set<std::string> seen;
    for (auto const& [punct, str] : table) {
      if (seen.count(str)) {
        throw std::runtime_error("duplicated punctuation: " + std::string(str));
      }
      seen.insert(str);
    }
  }

  class _debug_class_ {
  private:
    _debug_class_() { check_duplications(); }
  };

  static _debug_class_ const _;
#endif

} // namespace fire