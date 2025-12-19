#pragma once

#include <vector>

#include "Token.hpp"
#include "Source.hpp"

#include "Error.hpp"

namespace superman {
  class Lexer {
    friend class Parser;

    SourceCode& _source;
    size_t _pos;
    size_t const _len;

  public:
    Lexer(SourceCode& source) : _source(source), _pos(0), _len(source.get_len()) {}

    std::vector<Token> lex() {
      remove_all_comments();

      _pos = 0;

      auto tokens = std::vector<Token>();

      while (!is_end())
        tokens.emplace_back(tokenize(peek()));

      tokens.emplace_back(TokenKind::Eof, "", _source, _pos);

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

    //
    // - remove_all_comments
    // すべてのコメントを空白に置き換え
    // ( エラー表示時に行・列の番号が変わってしまうため、削除はしない )
    void remove_all_comments() {
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

    int find_comment(int begin) {
      for (int i = begin; i + 2 < static_cast<int>(_len); i++)
        if (match("//") || match("/*")) return i;

      return -1;
    }

  private:
    bool is_end() { return _pos >= _len; }

    char peek() { return _source[_pos]; }

    void pass_space() {
      while (!is_end() && isspace(peek()))
        _pos++;
    }

    bool match(std::string const& s) {
      return _pos + s.length() <= _len && _source.data.substr(_pos, s.length()) == s;
    }

    //
    // - erase
    //
    // 現在位置から文字を n 個削除する・または c に置き換える
    // 終端までに n 個分の文字がない場合は何もせず false を返す
    bool erase(int n, char c = -1) { // -1 = 削除
      if (_pos + n > _len) return false;

      if (c == -1)
        _source.data.erase(_source.data.begin() + _pos, _source.data.begin() + _pos + n);
      else
        std::fill(_source.data.begin() + _pos, _source.data.begin() + _pos + n, c);

      return true;
    }

    inline void replace(int n, char c) { erase(n, c); }

    Token tokenize(char c) {
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
        return Token(kind, std::string(str, len), _source, pos);
      }

      // a-z|A-Z|_
      else if (std::isalpha(c) || (c == '_')) {
        kind = TokenKind::Identifier;
        while (!is_end() && (std::isalnum((c = peek())) || c == '_'))
          _pos++, len++;
        pass_space();
        return Token(kind, std::string(str, len), _source, pos);
      }

      // char or string
      else if (c == '\'' || c == '"') {
        kind = c == '"' ? TokenKind::String : TokenKind::Char;
        _pos++;
        char x;
        std::string ss;
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
        return Token(kind, ss, _source, pos);
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
          return Token(TokenKind::Punctuator, s, _source, pos);
        }
      }

      throw err::invalid_token(Token(TokenKind::Unknown, std::string(1, c), _source, pos));
    }
  };
} // namespace superman