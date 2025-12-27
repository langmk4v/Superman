#include "Utils.hpp"
#include "Error.hpp"

#include "Lexer.hpp"

namespace fire {

  extern _token_punct_str_map_ const* _psmap_table_pointer_;
  extern volatile size_t _psmap_table_size_;

  static _token_punct_str_map_ const* find_punct(char const* ptr) {
    for (size_t i = 0; i < _psmap_table_size_; i++) {
      if (std::strncmp(ptr, _psmap_table_pointer_[i].str, std::strlen(_psmap_table_pointer_[i].str)) == 0) {
        return &_psmap_table_pointer_[i];
      }
    }
    return nullptr;
  }

  Token* Lexer::lex() {
    Token head;
    Token* cur = &head;

    pass_space();

    while (!is_end()) {
      while(match("//")) { pass_line_comment(); }
      while(match("/*")) { pass_block_comment(); }  
      pass_space();
      cur = tokenize(peek(), cur);
    }

    cur->next = new Token(TokenKind::Eof, "", cur, _source, _pos);

    size_t i = 0, line = 1, col = 1;

    for (Token* t = head.next; (t = t->next);) {
      for (; i < t->pos; i++, col++)
        if (get_char(i) == '\n') line++, col = 0;
      t->line = line;
      t->column = col;
    }

    return head.next;
  }

  Token* Lexer::tokenize(char c, Token* prev) {
    TokenKind kind = TokenKind::Unknown;
    char const* str = getptr();
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
      std::string* ss_ = new std::string(1, c);
      std::string&ss=*ss_;
      while (!is_end() && (x = peek()) != c) {
        if (x == '\\') {
          _pos++;
          switch (peek()) {
            case '0': x = 0; break;
            case 't': x = '\t'; break;
            case 'r': x = '\r'; break;
            case 'n': x = '\n'; break;
            case 'b': x = '\b'; break;
            default: todoimpl;
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
      ss_->push_back(c);
      return new Token(kind, std::string(ss_->data(),ss_->length()), prev, _source, pos);
    }

    else if (_token_punct_str_map_ const* p = find_punct(getptr()); p != nullptr) {
      _pos+=std::strlen(p->str);
      pass_space();
      auto tok= new Token(TokenKind::Punctuator, std::string(p->str), prev, _source, pos);
      tok->punct=p->punct;
      return tok;
    }

    char*buf=new char[2];
    buf[0]=c;
    buf[1]=0;

    throw err::invalid_token(
        *(new Token(TokenKind::Unknown, buf, prev, _source, pos)));
  }

} // namespace fire 