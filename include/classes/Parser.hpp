#pragma once

#include "Node.hpp"
#include "Lexer.hpp"
#include "Error.hpp"

namespace fire {
  class Parser {
    SourceFile& source;

    Token* cur;

  public:
    Parser(SourceFile& source, Token* _tok)
      : source(source), cur(_tok)
    {
    }

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

    NdTemplatableBase& _parse_template_param_defs(NdTemplatableBase& B) {
      if (eat("<")) {
        do {
          B.parameter_defs.emplace_back(ps_type_name());
        } while (!is_end() && eat(","));
        expect(">");
      }
      return B;
    }

    NdFunction* ps_function(bool is_method = false);

    NdClass* ps_class();

    NdEnumeratorDef* ps_enumerator_def();
    NdEnum* ps_enum();

    NdNamespace* ps_namespace();

    Node* ps_mod_item();
    NdModule* ps_mod();

    void ps_do_import(Token* import_token, std::string path);

    void ps_import();

    void merge_namespaces(std::vector<Node*>& items);

    void reorder_items(std::vector<Node*>& items);

    NdModule* parse();

  private:
    bool is_end() {
      return cur->is(TokenKind::Eof);
    }
    bool eat(char const* s) {
      return cur->text == s ? (next(), true) : false;
    }
    bool look(char const* s) {
      return cur->text == s;
    }
    Token* expect(char const* s) {
      if (cur->text != s) throw err::expected_but_found(*cur, s);
      return next();
    }
    Token* expect_ident() {
      if (cur->kind != TokenKind::Identifier) throw err::expected_identifier_tok(*cur);
      return next();
    }
    Token* next() {
      return (cur=cur->next)->prev;
    }
  };
} // namespace fire