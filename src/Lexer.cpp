#include "Utils.hpp"
#include "Lexer.hpp"
#include "Error.hpp"

namespace fire {

  std::vector<Token> Lexer::lex() {
    remove_all_comments();

    _pos = 0;

    auto tokens = std::vector<Token>();

    pass_space();

    while (!is_end())
      tokens.emplace_back(tokenize(peek()));

    tokens.emplace_back(TokenKind::Eof, "", &_source, _pos);

    for (size_t i = 0; i < tokens.size(); i++)
      tokens[i].index = i;

    size_t i = 0, line = 1, col = 1;
    for (auto& t : tokens) {
      for (; i < t.pos; i++, col++)
        if (_source[i] == '\n') line++, col = 0;

      t.line = line;
      t.column = col;
    }

    return tokens;
  }

  void Lexer::remove_all_comments() {
    while (_pos < _len) {
      if (match("//")) {
        replace(2, ' ');
        _pos += 2;
        while (!is_end() && !match("\n")) {
          replace(1, ' ');
          _pos++;
        }
      } else if (match("/*")) {
        replace(2, ' ');
        _pos += 2;
        while (!is_end()) {
          if (match("*/")) {
            replace(2, ' ');
            break;
          }
          replace(1, ' ');
          _pos++;
        }
      } else {
        _pos++;
      }
    }
  }

  Token Lexer::tokenize(char c) {
    pass_space();

    TokenKind kind = TokenKind::Unknown;
    char const* str = _source.data.data() + _pos;
    size_t len = 0;
    size_t pos = _pos;

    // 0-9
    if (std::isdigit(c)) {
      kind = TokenKind::Int;
      while (!is_end() && std::isdigit(peek()))
        _pos++, len++;
      if (peek() == '.') {
        // "." [0-9]* "f"?
        kind = TokenKind::Float;
        _pos++, len++;
        while (!is_end() && std::isdigit(peek()))
          _pos++, len++;
        if (peek() == 'f') _pos++, len++;
      }
      pass_space();
      return Token(kind, std::string(str, len), &_source, pos);
    }

    // a-z|A-Z|_
    else if (std::isalpha(c) || (c == '_')) {
      kind = TokenKind::Identifier;
      while (!is_end() && (std::isalnum((c = peek())) || c == '_'))
        _pos++, len++;
      pass_space();
      return Token(kind, std::string(str, len), &_source, pos);
    }

    // char or string
    else if (c == '\'' || c == '"') {
      kind = c == '"' ? TokenKind::String : TokenKind::Char;
      _pos++;
      char x;
      std::string ss = std::string(1, c);
      while (!is_end() && (x = peek()) != c) {
        if (x == '\\') {
          _pos++;
          switch (peek()) {
          case '0':
            x = 0;
            break;
          case 't':
            x = '\t';
            break;
          case 'r':
            x = '\r';
            break;
          case 'n':
            x = '\n';
            break;
          case 'b':
            x = '\b';
            break;
          default:
            alert;
            throw 10;
          }
        }
        ss += x;
        _pos++, len++;
      }
      if (peek() != c) {
        alert;
        throw 4545; // char or string literal not terminated
      }
      _pos++;
      pass_space();
      return Token(kind, ss + c, &_source, pos);
    }

    // punctuator
    static char const* _plist[] = {
        "##", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "::", "->", "=>",
        "#",  "$",  "`",  "+",  "-",  "*",  "/",  "%",  "|",  "&",  "~",  "^",  "=",  ",",
        ".",  ";",  ":",  "?",  "!",  "(",  ")",  "<",  ">",  "[",  "]",  "{",  "}",
    };

    for (std::string s : _plist) {
      if (match(s)) {
        _pos += s.length();
        pass_space();
        return Token(TokenKind::Punctuator, s, &_source, pos);
      }
    }

    throw err::invalid_token(Token(TokenKind::Unknown, std::string(1, c), &_source, pos));
  }

} // namespace fire