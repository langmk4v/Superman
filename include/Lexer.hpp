#pragma once

#include <vector>
#include <string>

#include "SourceFile.hpp"
#include "Token.hpp"

namespace fire {
  class Lexer {
    SourceFile const* _source;
    size_t _pos = 0;
    size_t const _len;

  public:
    Lexer(SourceFile const* source) : _source(source), _pos(0), _len(source->length) {}

    Token* lex();

    Token* tokenize(char c, Token* prev);

  private:
    bool is_end() { return _pos >= _len; }

    char peek() { return (*_source)[_pos]; }

    char get_char(size_t pos) { return (*_source)[pos]; }

    char const* getptr() { return _source->data.data() + _pos; }

    bool match(std::string_view s) {
      return _pos + s.length() <= _len &&
             std::strncmp(_source->data.data() + _pos, s.data(), s.length()) == 0;
    }

    bool consume(std::string_view s) { return match(s) ? (_pos += s.length(), true) : false; }

    void pass_space() {
      while (!is_end() && isspace(peek()))
        _pos++;
    }

    void pass_line_comment() {
      if (consume("//")) {
        while (!is_end() && peek() != '\n')
          _pos++;
      }
    }

    void pass_block_comment() {
      if (consume("/*")) {
        while (!is_end() && !consume("*/"))
          _pos++;
      }
    }
  };
} // namespace fire