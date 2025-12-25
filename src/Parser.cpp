#include <filesystem>

#include "Utils.hpp"
#include "Parser.hpp"
#include "Error.hpp"

namespace fire {

  NdSymbol* Parser::ps_symbol(bool as_typename) {
    auto sym = new NdSymbol(*expect_ident());
    Token* save = cur;

    if (eat("<")) {
      try {
        do {
          sym->te_args.emplace_back(ps_type_name());
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

    Token& tok = *cur;

    //
    // parse syntax suger for tuple type
    // (T, U, ...) --> tuple<T, U, ...>
    if (eat("(")) {
      NdSymbol* tuple_type = new NdSymbol(tok);

      tuple_type->name.kind = TokenKind::Identifier;
      tuple_type->name.text = "tuple";

      tuple_type->te_args.push_back(ps_type_name());

      expect(",");

      do {
        tuple_type->te_args.push_back(ps_type_name());
      } while (!is_end() && eat(","));

      expect(")");

      return tuple_type;
    }

    if (eat("decltype")) {
      expect("(");
      auto x = new NdDeclType(tok, ps_expr());
      expect(")");

      auto s = new NdSymbol(tok);
      s->dec = x;

      return s;
    }

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
        } while (!is_end() && eat(","));

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

    if (eat("self")) return new NdSelf(tok);

    if (eat("true")) {
      auto v = new NdValue(tok);
      v->obj = new ObjBool(true);
      return v;
    }

    if (eat("false")) {
      auto v = new NdValue(tok);
      v->obj = new ObjBool(false);
      return v;
    }

    if (eat("decltype")) {
      throw err::parses::cannot_use_decltype_here(tok);
    }

    if (cur->is(TokenKind::Identifier)) {
      return ps_symbol();
    }

    NdValue* v = new NdValue(*cur);

    switch (cur->kind) {
    case TokenKind::Int:
      v->obj = new ObjInt(std::stoll(cur->text));
      cur++;
      break;

    case TokenKind::Float:
      v->obj = new ObjFloat(std::stold(cur->text));
      cur++;
      break;

    case TokenKind::Char: {
      auto s16 = to_utf16(cur->text.substr(1, cur->text.size() - 2));
      if (s16.empty() || s16.size() > 1) {
        throw err::invalid_character_literal(*cur);
      }
      v->obj = new ObjChar(s16[0]);
      cur++;
      break;
    }

    case TokenKind::String:
      v->obj = new ObjString(to_utf16(cur->text.substr(1, cur->text.size() - 2)));
      cur++;
      break;

    default:
      throw err::invalid_syntax(*cur);
    }

    return v;
  }

  Node* Parser::ps_subscript() {
    auto& tok = *cur;

    auto x = ps_factor();

    while (!is_end()) {
      auto op = cur;
      if (eat("(")) {
        auto y = new NdCallFunc(x, *op);
        if (!eat(")")) {
          do {
            auto key = ps_expr();

            if (eat(":"))
              y->args.push_back(new NdKeyValuePair(*op, key, ps_expr()));
            else
              y->args.push_back(key);
          } while (eat(","));
          expect(")");
        }
        x = y;
      } else if (eat("[")) {

        auto index = ps_expr();

        if (eat(":")) {
          auto end = ps_expr();
          index = new NdExpr(NodeKind::Slice, tok, index, end);
        }

        x = new NdExpr(NodeKind::Subscript, *op, x, index);
        expect("]");
      } else if (eat(".")) {
        auto right = ps_factor();

        if (auto rr = right->as<NdCallFunc>(); right->is(NodeKind::CallFunc)) {
          rr->is_method_call = true;
          rr->inst_expr = x;
          rr->args.insert(rr->args.begin(), x);
          x = rr;
        } else {
          if (right->kind != NodeKind::Symbol) {
            throw err::invalid_syntax(*op);
          }
          x = new NdExpr(NodeKind::MemberAccess, *op, x, right);
        }
      } else
        break;
    }

    if (eat("++")) {
      return new NdInclement(tok, x, false);
    }
    if (eat("--")) {
      return new NdDeclement(tok, x, false);
    }

    return x;
  }

  Node* Parser::ps_unary() {
    auto& tok = *cur;

    if (eat("++")) {
      return new NdInclement(tok, ps_subscript(), true);
    }

    if (eat("--")) {
      return new NdDeclement(tok, ps_subscript(), true);
    }

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

    if (eat("&")) {
      auto node = new NdRef(tok);
      node->expr = ps_subscript();
      return node;
    }

    if (eat("*")) {
      auto node = new NdDeref(tok);
      node->expr = ps_subscript();
      return node;
    }

    if (eat("!")) {
      auto node = new NdNot(tok);
      node->expr = ps_subscript();
      return node;
    }

    if (eat("~")) {
      auto node = new NdBitNot(tok);
      node->expr = ps_subscript();
      return node;
    }

    if (eat("-")) {
      auto zero = new NdValue(tok);
      zero->obj = new ObjInt(0);
      return new NdExpr(NodeKind::Sub, tok, zero, ps_subscript());
    }

    eat("+");

    return ps_subscript();
  }

  Node* Parser::ps_terms() {
    auto x = ps_unary();
    while (!is_end()) {
      auto op = cur;

      if (eat("*"))
        x = new NdExpr(NodeKind::Mul, *op, x, ps_unary());
      else if (eat("/"))
        x = new NdExpr(NodeKind::Div, *op, x, ps_unary());
      else
        break;
    }
    return x;
  }

  Node* Parser::ps_add_sub() {
    auto x = ps_terms();
    while (!is_end()) {
      auto op = cur;
      if (eat("+"))
        x = new NdExpr(NodeKind::Add, *op, x, ps_terms());
      else if (eat("-"))
        x = new NdExpr(NodeKind::Sub, *op, x, ps_terms());
      else
        break;
    }
    return x;
  }

  Node* Parser::ps_shift() {
    auto x = ps_add_sub();
    while (!is_end()) {
      auto op = cur;
      if (eat("<<"))
        x = new NdExpr(NodeKind::LShift, *op, x, ps_add_sub());
      else if (eat(">>"))
        x = new NdExpr(NodeKind::RShift, *op, x, ps_add_sub());
      else
        break;
    }
    return x;
  }

  Node* Parser::ps_compare() {
    auto x = ps_shift();
    while (!is_end()) {
      auto op = cur;
      if (eat("<"))
        x = new NdExpr(NodeKind::Bigger, *op, ps_shift(), x);
      else if (eat(">"))
        x = new NdExpr(NodeKind::BiggerOrEqual, *op, x, ps_shift());
      else if (eat("<="))
        x = new NdExpr(NodeKind::Bigger, *op, ps_shift(), x);
      else if (eat(">="))
        x = new NdExpr(NodeKind::BiggerOrEqual, *op, x, ps_shift());
      else
        break;
    }
    return x;
  }

  Node* Parser::ps_equality() {
    auto x = ps_compare();
    while (!is_end()) {
      auto op = cur;
      if (eat("=="))
        x = new NdExpr(NodeKind::Equal, *op, x, ps_compare());
      else if (eat("!="))
        x = new NdExpr(NodeKind::Not, *op, new NdExpr(NodeKind::Equal, *op, x, ps_compare()), nullptr);
      else
        break;
    }
    return x;
  }

  Node* Parser::ps_bit_and() {
    auto x = ps_equality();

    while (!is_end()) {
      auto op = cur;
      if (eat("&"))
        x = new NdExpr(NodeKind::BitAnd, *op, x, ps_equality());
      else
        break;
    }

    return x;
  }

  Node* Parser::ps_bit_xor() {
    auto x = ps_bit_and();

    while (!is_end()) {
      auto op = cur;
      if (eat("^"))
        x = new NdExpr(NodeKind::BitXor, *op, x, ps_bit_and());
      else
        break;
    }

    return x;
  }

  Node* Parser::ps_bit_or() {
    auto x = ps_bit_xor();

    while (!is_end()) {
      auto op = cur;
      if (eat("|"))
        x = new NdExpr(NodeKind::BitOr, *op, x, ps_bit_xor());
      else
        break;
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

  Node* Parser::ps_expr() { return ps_assign(); }

  NdLet* Parser::ps_let(bool expect_semi) {
    auto& tok = *expect("var");

    NdLet* let = new NdLet(tok, *cur);

    let->is_static = eat("static");

    if (eat("(")) {
      do {
        let->placeholders.push_back(expect_ident());
      } while (!is_end() && eat(","));
      expect(")");
    } else
      expect_ident();

    if (eat(":")) let->type = ps_type_name();
    if (eat("=")) let->init = ps_expr();
    if (expect_semi) expect(";");

    return let;
  }

  Node* Parser::ps_stmt() {
    auto& tok = *cur;

    if (look("{")) return ps_scope();

    if (look("var")) return ps_let();

    if (eat("if")) {
      auto x = new NdIf(tok);
      if (look("var")) x->vardef = ps_let(false);
      if (!x->vardef || eat(";")) x->cond = ps_expr();
      x->thencode = ps_stmt();
      if (eat("else")) x->elsecode = ps_stmt();
      return x;
    }

    if (eat("while")) {
      auto x = new NdWhile(tok);
      if (look("var")) x->vardef = ps_let(false);
      if (!x->vardef || eat(";")) x->cond = ps_expr();
      x->body = ps_scope();
      return x;
    }

    if (eat("for")) {
      auto x = new NdFor(tok);
      x->iter = *expect_ident();
      expect("in");
      x->iterable = ps_expr();
      x->body = ps_scope();
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
      auto public_flag = eat("pub");

      // method
      if (look("fn")) node->methods.emplace_back(ps_function(true))->is_pub = public_flag;

      // field
      else if (look("var"))
        node->fields.emplace_back(ps_let())->is_pub = public_flag;

      else if (eat("new")) {
        if (!public_flag) {
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
        node->m_new = newfn;
      } else if (eat("delete")) {
        todoimpl;
      } else {
        expect("}");
        break;
      }
    }
    return node;
  }

  NdEnum* Parser::ps_enum() { todoimpl; }

  NdNamespace* Parser::ps_namespace() {
    Token* tok = expect("namespace");

    std::string name = expect_ident()->text;

    NdNamespace* ns = new NdNamespace(*tok, name);

    while (eat("::")) {
      todo;
    }

    Token* scope_tok = expect("{");

    if (eat("}")) return ns;

    while (!is_end()) {
      ns->items.emplace_back(ps_mod_item());
      if (eat("}")) return ns;
    }

    throw err::scope_not_terminated(*scope_tok);
  }

  Node* Parser::ps_mod_item() {
    if (look("var")) return ps_let();
    if (look("fn")) return ps_function();
    if (look("class")) return ps_class();
    if (look("enum")) return ps_enum();
    if (look("namespace")) return ps_namespace();

    throw err::expected_item_of_module(*cur);
  }

  void Parser::ps_import() {

    auto import_token = cur;

    std::string path = expect_ident()->text;

    while (!is_end() && eat("::")) {
      path += "/";

      if (eat("*")) {
        todo;
      }

      path += expect_ident()->text;
    }

    expect(";");

    path = std::filesystem::absolute(source.get_folder() + path).string();

    if (!std::filesystem::exists(path)) {
      if (std::filesystem::exists(path + ".fire")) {
        path += ".fire";

        auto imported = source.import(path);

        if (imported->get_depth() >= 4) {
          throw err::parses::import_depth_limit_exceeded(*import_token, imported->path);
        }
      } else {
        throw err::parses::cannot_open_file(*import_token, path);
      }
    } else if (std::filesystem::is_directory(path)) {
      source.import_directory(path);
    }
  }

  NdModule* Parser::ps_mod() {
    NdModule* mod = new NdModule(*cur);

    while (!is_end() && eat("import")) {
      ps_import();
    }

    for (auto&& src : source.imports) {
      if (src->imported_nodes) continue;

      auto submod = src->parse();

      for (auto&& item : submod->items) {
        mod->items.emplace_back(item);
      }

      src->imported_nodes = true;
    }

    while (!is_end()) {
      if (look("import")) {
        throw err::parses::import_not_allowed_here(*cur);
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

  NdModule* Parser::parse() { return ps_mod(); }

} // namespace fire