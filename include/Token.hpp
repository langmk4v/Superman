#pragma once

#include <cstring>
#include <cstddef>

#include <string>

#include "SourceFile.hpp"

namespace fire {
  struct Object;

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

  enum TokenPunctuators {
    Punct_None,

    // symbol
    Punct_ScopeResol, // ::

    // assignments
    Punct_Assign,       // =
    Punct_AddAssign,    // +=
    Punct_SubAssign,    // -=
    Punct_MulAssign,    // *=
    Punct_DivAssign,    // /=
    Punct_ModAssign,    // %=
    Punct_AndAssign,    // &=
    Punct_OrAssign,     // |=
    Punct_XorAssign,    // ^=
    Punct_LShiftAssign, // <<=
    Punct_RShiftAssign, // >>=

    // compare
    Punct_Equal,          // ==
    Punct_NotEqual,       // !=
    Punct_Greater,        // >
    Punct_GreaterOrEqual, // >=
    Punct_Less,           // <
    Punct_LessOrEqual,    // <=
    Punct_And,            // &&
    Punct_Or,             // ||
    Punct_Not,            // !

    // arithmetics
    Punct_Add,    // +
    Punct_Sub,    // -
    Punct_Mul,    // *
    Punct_Div,    // /
    Punct_Mod,    // %
    Punct_LShift, // <<
    Punct_RShift, // >>

    // bit-calculation
    Punct_BitAnd, // &
    Punct_BitOr,  // |
    Punct_BitXor, // ^

    // other
    Punct_Inclement, // ++
    Punct_Declement, // --

    // misc
    Punct_RightArrow,   // ->
    Punct_Comma,        // ,
    Punct_Semicolon,    // ;
    Punct_Colon,        // :
    Punct_Dot,          // .
    Punct_Question,     // ?
    Punct_ParenOpen,    // (
    Punct_ParenClose,   // )
    Punct_BracketOpen,  // [
    Punct_BracketClose, // ]
    Punct_CurlyOpen,    // {
    Punct_CurlyClose,   // }
    Punct_Hash,         // #
    Punct_Dollar,       // $
    Punct_Backtick,     // `
    Punct_Tilde,        // ~
  };

  struct Token {
    TokenKind kind;

    TokenPunctuators punct = TokenPunctuators::Punct_None;

    std::string text;

    Token* prev = nullptr;
    Token* next = nullptr;

    SourceFile const* source = nullptr;
    size_t pos = 0;
    size_t line = 0;
    size_t column = 0;

    Object* literal_obj = nullptr;

    Token(TokenKind kind = TokenKind::Unknown) : kind(kind) {}

    Token(TokenKind k, std::string const& text, Token* prev, SourceFile const* src, size_t pos)
        : kind(k), text(text), prev(prev), source(src), pos(pos) {
      if (prev) prev->next = this;
    }

    static Token from_str(std::string const& text, TokenKind kind = TokenKind::Identifier) {
      return Token(kind, text, nullptr, SourceFile::GetCurrentReferenced(), 0);
    }

    bool is(TokenKind k) const { return kind == k; }

    bool is_punct(TokenPunctuators p) const { return punct == p; }

    bool is_literal() const { return TokenKind::Int <= kind && kind <= TokenKind::String; }

    static TokenPunctuators get_punct(char const* str);

    static char const* GetStringOfPunctuator(TokenPunctuators p);
  };

  struct _token_punct_str_map_ {
    TokenPunctuators punct;
    char const* str;
  };

} // namespace fire