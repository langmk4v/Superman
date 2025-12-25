#pragma once

#include <string>

namespace fire {
  enum class TokenKind {
    Unknown,
    Int,
    Float,
    Boolean,
    Char,
    String,
    Identifier,
    Punctuator,
    Eof,
  };

  struct SourceCode;

  struct Token {
    TokenKind kind;

    std::string text;

    SourceCode* source = nullptr;
    size_t pos = 0;
    size_t line = 0;
    size_t column = 0;

    size_t index = 0;

    Token(TokenKind kind = TokenKind::Unknown) : kind(kind) {}

    Token(TokenKind k, std::string s, SourceCode* src, size_t pos)
        : kind(k), text(std::move(s)), source(src), pos(pos) {}

    bool is(TokenKind k) const { return kind == k; }

    bool is_value() const { return TokenKind::Int <= kind && kind <= TokenKind::String; }
  };
} // namespace fire