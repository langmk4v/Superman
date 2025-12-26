#include "Utils.hpp"
#include "Error.hpp"

#include "Lexer.hpp"

namespace fire {

  Token* Lexer::lex() {
    Token head;
    Token* cur = &head;

    pass_space();

    while (!is_end()) {
      cur = tokenize(peek(), cur);
    }

    cur = new Token(TokenKind::Eof, std::string_view(), cur, _source, _pos);

    size_t i = 0, line = 1, col = 1;

    for (Token* t = cur; t; t = t->next) {
      for (; i < t->pos; i++, col++)
        if (get_char(i) == '\n') line++, col = 0;
      t->line = line;
      t->column = col;
    }

    return head.next;
  }

  Token* Lexer::tokenize(char c, Token* prev) {
    pass_space();

    TokenKind kind = TokenKind::Unknown;
    char const* str = _source->data + _pos;
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
      return new Token(kind, std::string(str, len), prev, _source, pos);
    }

    // a-z|A-Z|_
    else if (std::isalpha(c) || (c == '_')) {
      kind = TokenKind::Identifier;
      while (!is_end() && (std::isalnum((c = peek())) || c == '_'))
        _pos++, len++;
      pass_space();
      return new Token(kind, std::string(str, len), prev, _source, pos);
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
      return new Token(kind, ss + c, prev, _source, pos);
    }

    // punctuator
    static char const* _plist[] = {
        // assign with op
        ">>=", "<<=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",

        "##",  "++",  "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "::", "->", "=>",
        "#",   "$",   "`",  "+",  "-",  "*",  "/",  "%",  "|",  "&",  "~",  "^",  "=",  ",",
        ".",   ";",   ":",  "?",  "!",  "(",  ")",  "<",  ">",  "[",  "]",  "{",  "}",
    };

    for (std::string s : _plist) {
      if (match(s)) {
        _pos += s.length();
        pass_space();
        return new Token(TokenKind::Punctuator, s, prev, _source, pos);
      }
    }

    throw err::invalid_token(
        *(new Token(TokenKind::Unknown, std::string(1, c), prev, _source, pos)));
  }

} // namespace fire