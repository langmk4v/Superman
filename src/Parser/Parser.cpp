#include "Utils/macro.h"
#include "Utils/Strings.hpp"

#include "VM/Interp/Object.hpp"
#include "Parser/Parser.hpp"
#include "Driver/Error.hpp"

namespace fire::parser {

  using namespace lexer;

  NdSymbol* Parser::ps_symbol(bool as_typename) {
    auto sym = new NdSymbol(*expect_ident());
    Token* save = cur;

    if (eat("<")) {
      try {
        do {
          sym->te_args.emplace_back(ps_symbol());
        } while (!is_end() && eat(","));
        if (look(">>"))
          cur->text = ">";
        else
          expect(">");
      } catch (...) {
        sym->te_args.clear();
        cur = save;
      }
    }

    if (eat("::")) sym->next = ps_symbol();

    if (as_typename) {
      if (eat(":")) sym->concept_nd = ps_type_name();
      sym->is_ref = eat("ref");
      sym->is_const = eat("const");
    }

    return sym;
  }

  NdSymbol* Parser::ps_type_name() {
    return ps_symbol(true);
  }

  Node* Parser::ps_factor() {
    auto& tok = *cur;

    if (eat("(")) {
      auto node = ps_expr();

      if (eat(",")) {
        auto tu = new NdTuple(tok);
        tu->elems.push_back(node);

        do {
          tu->elems.push_back(ps_expr());
        } while (eat(","));

        node = tu;
      }

      expect(")");
      return node;
    }

    if (eat("[")) {
      auto node = new NdArray(tok);

      if (eat("]")) return node;

      do {
        node->data.emplace_back(ps_expr());
      } while (eat(","));

      expect("]");

      return node;
    }

    if (cur->is(TokenKind::Identifier)) {
      return ps_symbol();
    }

    NdValue* v = new NdValue(*cur);

    switch (cur->kind) {
      using namespace vm::interp;

      case TokenKind::Int:
        v->obj = new ObjInt(std::stoll(cur->text));
        cur++;
        break;

      case TokenKind::Float:
        v->obj = new ObjFloat(std::stold(cur->text));
        cur++;
        break;

      case TokenKind::String:
        v->obj = new ObjString(strings::to_utf16(cur->text));
        cur++;
        break;

      default:
        throw err::invalid_syntax(*cur);
    }

    return v;
  }

  Node* Parser::ps_subscript() {
    auto x = ps_factor();
    while (!is_end()) {
      auto op = cur;
      if (eat("(")) {
        auto y = new NdCallFunc(x, *op);
        if (!eat(")")) {
          do {
            y->args.emplace_back(ps_expr());
          } while (eat(","));
          expect(")");
        }
        x = y;
      } else if (eat("[")) {
        x = new NdExpr(NodeKind::Subscript, *op, x, ps_expr());
        expect("]");
      } else if (eat(".")) {
        x = new NdExpr(NodeKind::MemberAccess, *op, x, ps_factor());
      } else
        break;
    }
    return x;
  }

  Node* Parser::ps_unary() {
    auto& tok = *cur;

    if (eat("new")) {
      auto node = new NdNew(tok);
      node->type = ps_type_name();
      if (eat("(") && !eat(")")) {
        do {
          node->args.push_back(ps_expr());
        } while (eat(","));
        expect(")");
      }
      return node;
    }

    if (eat("delete")) {
      todoimpl;
    }

    return ps_subscript();
  }

  Node* Parser::ps_terms() {
    auto x = ps_unary();
    while (!is_end()) {
      auto op = cur;

      if (eat("*")) x = new NdExpr(NodeKind::Mul, *op, x, ps_unary());
      else if (eat("/")) x = new NdExpr(NodeKind::Div, *op, x, ps_unary());
      else break;
    }
    return x;
  }

  Node* Parser::ps_add_sub() {
    auto x = ps_terms();
    while (!is_end()) {
      auto op = cur;
      if (eat("+")) x = new NdExpr(NodeKind::Add, *op, x, ps_terms());
      else if (eat("-")) x = new NdExpr(NodeKind::Sub, *op, x, ps_terms());
      else break;
    }
    return x;
  }

  Node* Parser::ps_shift() {
    auto x = ps_add_sub();
    while (!is_end()) {
      auto op = cur;
      if (eat("<<")) x = new NdExpr(NodeKind::LShift, *op, x, ps_add_sub());
      else if (eat(">>")) x = new NdExpr(NodeKind::RShift, *op, x, ps_add_sub());
      else break;
    }
    return x;
  }

  Node* Parser::ps_compare() {
    auto x = ps_shift();
    while (!is_end()) {
      auto op = cur;
      if (eat("<")) x = new NdExpr(NodeKind::Bigger, *op, ps_shift(), x);
      else if (eat(">")) x = new NdExpr(NodeKind::BiggerOrEqual, *op, x, ps_shift());
      else if (eat("<=")) x = new NdExpr(NodeKind::Bigger, *op, ps_shift(), x);
      else if (eat(">=")) x = new NdExpr(NodeKind::BiggerOrEqual, *op, x, ps_shift());
      else break;
    }
    return x;
  }

  Node* Parser::ps_equality() {
    auto x = ps_compare();
    while (!is_end()) {
      auto op = cur;
      if (eat("==")) x = new NdExpr(NodeKind::Equal, *op, x, ps_compare());
      else if (eat("!=")) x = new NdExpr(NodeKind::Not, *op, new NdExpr(NodeKind::Equal, *op, x, ps_compare()), nullptr);
      else break;
    }
    return x;
  }

  Node* Parser::ps_bit_and() {
    auto x = ps_equality();

    while (!is_end()) {
      auto op = cur;
      if (eat("&")) x = new NdExpr(NodeKind::BitAnd, *op, x, ps_equality());
      else break;
    }

    return x;
  }

  Node* Parser::ps_bit_xor() {
    auto x = ps_bit_and();

    while (!is_end()) {
      auto op = cur;
      if (eat("^")) x = new NdExpr(NodeKind::BitXor, *op, x, ps_bit_and());
      else break;
    }

    return x;
  }

  Node* Parser::ps_bit_or() {
    auto x = ps_bit_xor();

    while (!is_end()) {
      auto op = cur;
      if (eat("|")) x = new NdExpr(NodeKind::BitOr, *op, x, ps_bit_xor());
      else break;
    }

    return x;
  }

  Node* Parser::ps_log_and() {
    auto x = ps_bit_or();

    while (!is_end() && eat("&&"))
      x = new NdExpr(NodeKind::LogAnd, *(cur - 1), x, ps_bit_or());

    return x;
  }

  Node* Parser::ps_log_or() {
    auto x = ps_log_and();

    while (!is_end() && eat("||"))
      x = new NdExpr(NodeKind::LogOr, *(cur - 1), x, ps_log_and());

    return x;
  }

  Node* Parser::ps_assign() {
    auto x = ps_log_or();
    auto op = cur;

    if (eat("=")) x = new NdExpr(NodeKind::Assign, *op, x, ps_assign());

    return x;
  }

  Node* Parser::ps_expr() {
    return ps_assign();
  }

  NdLet* Parser::ps_let(bool from_var_kwd) {
    auto& tok = *expect(from_var_kwd ? "var" : "let");
    auto x = new NdLet(tok, *expect_ident());
    if (eat(":")) x->type = ps_type_name();
    if (eat("=")) x->init = ps_expr();
    expect(";");
    return x;
  }

  Node* Parser::ps_stmt() {
    auto& tok = *cur;

    if (look("{")) return ps_scope();

    if (look("let")) return ps_let();

    if (eat("if")) {
      auto x = new NdIf(tok);
      x->cond = ps_expr();
      x->thencode = ps_stmt();
      if (eat("else")) x->elsecode = ps_stmt();
      return x;
    }

    if (eat("return")) {
      auto nd = new NdReturn(tok);
      if (!eat(";")) nd->expr = ps_expr(), expect(";");
      return nd;
    }

    if (eat("break")) {
      expect(";");
      return new NdBreakOrContinue(NodeKind::Break, tok);
    }

    if (eat("continue")) {
      expect(";");
      return new NdBreakOrContinue(NodeKind::Continue, tok);
    }

    auto x = ps_expr();
    expect(";");
    return x;
  }

  NdScope* Parser::ps_scope() {
    auto x = new NdScope(*expect("{"));

    if (!eat("}")) {
      while (!is_end()) {
        x->items.emplace_back(ps_stmt());
        if (eat("}")) return x;
      }
      throw err::scope_not_terminated(x->token);
    } else
      return x;
  }

  NdFunction* Parser::ps_function(bool is_method) {
    auto& tok = *expect("fn");
    auto node = new NdFunction(tok, *expect_ident());

    _parse_template_param_defs(*node);

    if (eat("(")) {
      if (!eat(")")) {

        if (is_method && eat("self")) {
          node->take_self = true;
          if (!eat(",")) goto L_fn_pass_args;
        }

        do {
          auto& A = node->args.emplace_back(*expect_ident(), nullptr);
          expect(":");
          A.type = ps_type_name();
        } while (!is_end() && eat(","));
      L_fn_pass_args:
        expect(")");
      }
    }

    node->result_type = eat("->") ? ps_type_name() : nullptr;

    node->body = ps_scope();

    return node;
  }

  NdClass* Parser::ps_class() {
    auto& tok = *expect("class");
    auto node = new NdClass(tok, *expect_ident());
    _parse_template_param_defs(*node);

    if (eat(":")) node->base_class = ps_type_name();

    expect("{");
    if (eat("}")) throw err::empty_class_or_enum_is_not_valid(tok);
    while (!is_end()) {
      auto atepub = eat("pub");

      if (look("fn"))
        node->methods.emplace_back(ps_function(true))->is_pub = atepub;
      else if (look("var"))
        node->fields.emplace_back(ps_let(true))->is_pub = atepub;
      else if (eat("new")) {
        if (!atepub) {
          warns::added_pub_attr_automatically (*(cur - 1))();
          warns::show_note(*(cur - 1), "insert 'pub' keyword to remove this warning messages.")();
        }

        if (node->m_new) throw err::duplicate_of_definition(*(cur - 1), node->m_new->token);
        auto newfn = new NdFunction(*(cur - 1), *(cur - 1));
        if (eat("(") && !eat(")")) {
          do {
            auto& A = newfn->args.emplace_back(*expect_ident(), nullptr);
            expect(":");
            A.type = ps_type_name();
          } while (eat(","));
          expect(")");
        }
        if (look("->")) {
          throw err::cannot_specify_return_type_of_constructor(*cur);
        }
        newfn->body = ps_scope();
      } else if (eat("delete")) {
        todoimpl;
      } else {
        expect("}");
        break;
      }
    }
    return node;
  }

  NdEnum* Parser::ps_enum() {
    todoimpl;
  }

  Node* Parser::ps_mod_item() {
    if (look("let")) return ps_let();
    if (look("fn")) return ps_function();
    if (look("class")) return ps_class();
    if (look("enum")) return ps_enum();

    throw err::expected_item_of_module(*cur);
  }

  NdModule* Parser::ps_mod() {
    NdModule* mod = new NdModule(*cur);

    while (!is_end()) {
      if (look("import")) {
        todoimpl;
      }

      auto item = mod->items.emplace_back(ps_mod_item());

      if (item->is(NodeKind::Function)) {
        if (auto f = item->as<NdFunction>(); f->name.text == "main") {
          if (mod->main_fn) {
            throw err::duplicate_of_definition(f->name, mod->main_fn->name);
          } else {
            mod->main_fn = f;
          }
        }
      }
    }

    return mod;
  }

  NdModule* Parser::parse() {
    return ps_mod();
  }

} // namespace superman