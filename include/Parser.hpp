#pragma once

#include "Error.hpp"
#include "Lexer.hpp"
#include "Node.hpp"

namespace fire {
class Parser {
  SourceFile& source;

  Token* cur;

public:
  Parser(SourceFile& source, Token* _tok) : source(source), cur(_tok) {}

  NdSymbol* ps_symbol(bool as_typename = false);
  NdSymbol* ps_type_name();

  Node* ps_factor();
  Node* ps_subscript();
  Node* ps_unary();
  Node* ps_terms();
  Node* ps_add_sub();
  Node* ps_shift();
  Node* ps_compare();
  Node* ps_equality();
  Node* ps_bit_and();
  Node* ps_bit_xor();
  Node* ps_bit_or();
  Node* ps_log_and();
  Node* ps_log_or();
  Node* ps_assign();
  Node* ps_expr();

  NdLet* ps_let(bool expect_semi = true);
  Node* ps_stmt();
  NdScope* ps_scope();

  NdFunction* ps_function(bool is_method = false);
  NdClass* ps_class();
  NdEnumeratorDef* ps_enumerator_def();
  NdEnum* ps_enum();
  NdNamespace* ps_namespace();
  Node* ps_toplevel();

  NdModule* ps_mod();

  NdModule* parse();

private:
  void ps_import();
  void ps_do_import(Token* import_token, std::string path);

  void merge_namespaces(std::vector<Node*>& items);

  void reorder_items(std::vector<Node*>& items);

  NdTemplatableBase& _parse_template_param_defs(NdTemplatableBase& B) {
    if (eat("<")) {
      do {
        B.parameter_defs.emplace_back(ps_type_name());
      } while (!is_end() && eat(","));
      expect(">");
    }
    return B;
  }

  bool is_end() { return cur->is(TokenKind::Eof); }

  bool eat(char const* s) { return cur->text == s ? (next(), true) : false; }
  bool eat(TokenPunctuators p) {
    return cur->punct == p ? (next(), true) : false;
  }

  bool eat_dot() { return eat(TokenPunctuators::Punct_Dot); }
  bool eat_comma() { return eat(TokenPunctuators::Punct_Comma); }
  bool eat_colon() { return eat(TokenPunctuators::Punct_Colon); }
  bool eat_semi() { return eat(TokenPunctuators::Punct_Semicolon); }

  // ( )
  bool eat_paren_open() { return eat(TokenPunctuators::Punct_ParenOpen); }
  bool eat_paren_close() { return eat(TokenPunctuators::Punct_ParenClose); }

  // [ ]
  bool eat_bracket_open() { return eat(TokenPunctuators::Punct_BracketOpen); }
  bool eat_bracket_close() { return eat(TokenPunctuators::Punct_BracketClose); }

  // { }
  bool eat_curly_open() { return eat(TokenPunctuators::Punct_CurlyOpen); }
  bool eat_curly_close() { return eat(TokenPunctuators::Punct_CurlyClose); }

  // < >
  bool eat_angle_open() { return eat(TokenPunctuators::Punct_Less); }
  bool eat_angle_close() { return eat(TokenPunctuators::Punct_Greater); }

  bool look(char const* s) { return cur->text == s; }
  bool look(TokenPunctuators p) { return cur->punct == p; }

  Token* expect(char const* s) {
    if (cur->text != s) throw err::expected_but_found(*cur, s);
    return next();
  }

  Token* expect(TokenPunctuators p) {
    if (cur->punct != p)
      throw err::expected_but_found(*cur, Token::GetStringOfPunctuator(p));
    return next();
  }

  Token* expect_ident() {
    if (cur->kind != TokenKind::Identifier)
      throw err::expected_identifier_tok(*cur);
    return next();
  }

  Token* expect_comma() { return expect(TokenPunctuators::Punct_Comma); }
  Token* expect_colon() { return expect(TokenPunctuators::Punct_Colon); }
  Token* expect_semi() { return expect(TokenPunctuators::Punct_Semicolon); }
  Token* expect_paren_open() {
    return expect(TokenPunctuators::Punct_ParenOpen);
  }
  Token* expect_paren_close() {
    return expect(TokenPunctuators::Punct_ParenClose);
  }
  Token* expect_bracket_open() {
    return expect(TokenPunctuators::Punct_BracketOpen);
  }
  Token* expect_bracket_close() {
    return expect(TokenPunctuators::Punct_BracketClose);
  }
  Token* expect_curly_open() {
    return expect(TokenPunctuators::Punct_CurlyOpen);
  }
  Token* expect_curly_close() {
    return expect(TokenPunctuators::Punct_CurlyClose);
  }
  Token* expect_angle_open() { return expect(TokenPunctuators::Punct_Less); }
  Token* expect_angle_close() {
    return expect(TokenPunctuators::Punct_Greater);
  }

  Token* next() { return (cur = cur->next)->prev; }
};
} // namespace fire