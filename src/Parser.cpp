#include <filesystem>

#include "Utils.hpp"
#include "Driver.hpp"
#include "Parser.hpp"
#include "Error.hpp"
#include "FileSystem.hpp"

#include "strconv.h"

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

    if (Token* tok = cur; eat("::")) {
      sym->scope_resol_tok = tok;
      sym->next = ps_symbol();
    }

    if (as_typename) {
      if (eat(":"))
        sym->concept_nd = ps_type_name();
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

      if (eat("]"))
        return node;

      do {
        node->data.emplace_back(ps_expr());
      } while (eat(","));

      expect("]");

      return node;
    }

    if (eat("self"))
      return new NdSelf(tok);

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
        v->obj = new ObjInt(std::atoll(cur->text.data()));
        next();
        break;

      case TokenKind::Float:
        v->obj = new ObjFloat(std::atof(cur->text.data()));
        next();
        break;

      case TokenKind::Char: {
        std::u16string s16 = utf8_to_utf16_len_cpp(cur->text.data() + 1, cur->text.length() - 2);

        if (s16.empty() || s16.size() > 1) {
          throw err::invalid_character_literal(*cur);
        }
        v->obj = new ObjChar(s16[0]);
        next();
        break;
      }

      case TokenKind::String: {
        v->obj = ObjString::from_char16_ptr_move(utf8_to_utf16_with_len(nullptr,cur->text.data()+1, cur->text.length()-2));
        next();
        break;
      }

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
        if(x->is(NodeKind::MemberAccess)){
          y->is_method_call = true;
          y->inst_expr = x->as<NdExpr>()->lhs;
          y->callee = x->as<NdExpr>()->rhs;
        }
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
        // "[:end]"
        if (eat(":")) {
          auto end = ps_expr();
          x = new NdExpr(NodeKind::Subscript, tok, x,
                         new NdExpr(NodeKind::Slice, tok, nullptr, end));
          expect("]");
        } else {
          auto index = ps_expr();

          if (eat(":")) {
            auto end = look("]") ? nullptr : ps_expr();
            x = new NdExpr(NodeKind::Subscript, tok, x,
                           new NdExpr(NodeKind::Slice, tok, index, end));
          } else
            x = new NdExpr(NodeKind::Subscript, tok, x, index);

          expect("]");
        }
      } else if (eat(".")) {
        // get tuple element:
        // a.<N>
        if (eat("<")) {
          if (cur->kind != TokenKind::Int) {
            throw err::expected_but_found(*cur, "int");
          }
          x = new NdGetTupleElement(tok, x, std::atoi(cur->text.data()));
          x->as<NdGetTupleElement>()->index_tok = cur;
          next();
          expect(">");
          continue;
        }
        else {
          auto right = ps_factor();
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

    /*
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
    }*/

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
        x = new NdExpr(NodeKind::Not, *op, new NdExpr(NodeKind::Equal, *op, x, ps_compare()),
                       nullptr);
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
      x = new NdExpr(NodeKind::LogAnd, *cur->prev, x, ps_bit_or());

    return x;
  }

  Node* Parser::ps_log_or() {
    auto x = ps_log_and();

    while (!is_end() && eat("||"))
      x = new NdExpr(NodeKind::LogOr, *cur->prev, x, ps_log_and());

    return x;
  }

  Node* Parser::ps_assign() {
    auto x = ps_log_or();
    auto op = cur;

    if (eat("="))
      x = new NdExpr(NodeKind::Assign, *op, x, ps_assign());

    if (eat("+="))
      x = new NdAssignWithOp(NodeKind::Add, *op, x, ps_assign());
    if (eat("-="))
      x = new NdAssignWithOp(NodeKind::Sub, *op, x, ps_assign());
    if (eat("*="))
      x = new NdAssignWithOp(NodeKind::Mul, *op, x, ps_assign());
    if (eat("/="))
      x = new NdAssignWithOp(NodeKind::Div, *op, x, ps_assign());
    if (eat("%="))
      x = new NdAssignWithOp(NodeKind::Mod, *op, x, ps_assign());
    if (eat("&="))
      x = new NdAssignWithOp(NodeKind::BitAnd, *op, x, ps_assign());
    if (eat("|="))
      x = new NdAssignWithOp(NodeKind::BitOr, *op, x, ps_assign());
    if (eat("^="))
      x = new NdAssignWithOp(NodeKind::BitXor, *op, x, ps_assign());
    if (eat("<<="))
      x = new NdAssignWithOp(NodeKind::LShift, *op, x, ps_assign());
    if (eat(">>="))
      x = new NdAssignWithOp(NodeKind::RShift, *op, x, ps_assign());

    return x;
  }

  Node* Parser::ps_expr() {
    return ps_assign();
  }

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

    if (eat(":"))
      let->type = ps_type_name();
    if (eat("="))
      let->init = ps_expr();
    if (expect_semi)
      expect(";");

    return let;
  }

  Node* Parser::ps_stmt() {
    auto& tok = *cur;

    if (look("{"))
      return ps_scope();

    if (look("var"))
      return ps_let();

    if (eat("try")) {
      auto x = new NdTry(tok);

      x->body = ps_scope();

      if (!look("catch")) {
        throw err::parses::expected_catch_block(*cur->prev);
      }

      while (!is_end() && eat("catch")) {
        auto catch_ = new NdCatch(tok);
        catch_->holder = *expect_ident();
        expect(":");
        catch_->error_type = ps_type_name();
        catch_->body = ps_scope();
        x->catches.emplace_back(catch_);
      }

      if (eat("finally")) {
        x->finally_block = ps_scope();
      }

      return x;
    }

    if (eat("if")) {
      auto x = new NdIf(tok);
      if (look("var"))
        x->vardef = ps_let(false);
      if (!x->vardef || eat(";"))
        x->cond = ps_expr();
      x->thencode = ps_scope();
      if (eat("else")) {
        if (look("if"))
          x->elsecode = ps_stmt();
        else
          x->elsecode = ps_scope();
      }
      return x;
    }

    if (eat("while")) {
      auto x = new NdWhile(tok);
      if (look("var"))
        x->vardef = ps_let(false);
      if (!x->vardef || eat(";"))
        x->cond = ps_expr();
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
      if (!eat(";"))
        nd->expr = ps_expr(), expect(";");
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
        if (eat("}"))
          return x;
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
          if (!eat(","))
            goto L_fn_pass_args;
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

    if (eat(":"))
      node->base_class = ps_type_name();

    expect("{");

    if (eat("}"))
      throw err::empty_class_or_enum_is_not_valid(tok);

    while (!is_end()) {
      auto public_flag = eat("pub");

      // method
      if (look("fn")) {
        auto M = node->methods.emplace_back(ps_function(true));

        M->is_pub = public_flag;
        M->take_self = true;
      }

      // field
      else if (look("var"))
        node->fields.emplace_back(ps_let())->is_pub = public_flag;

      else if (eat("new")) {
        if (!public_flag) {
          warns::added_pub_attr_automatically (*cur->prev)();
          warns::show_note(*cur->prev, "insert 'pub' keyword to remove this warning messages.")();
        }

        if (node->m_new)
          throw err::duplicate_of_definition(*cur->prev, node->m_new->token);
        auto newfn = new NdFunction(*cur->prev, *cur->prev);
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

  NdEnumeratorDef* Parser::ps_enumerator_def() {
    Token* tok = expect_ident();

    NdEnumeratorDef* nd = new NdEnumeratorDef(*tok);

    nd->name = *tok;

    if (eat("(")) {
      if (cur->kind == TokenKind::Identifier && cur->next->text == ":") {
        nd->type = NdEnumeratorDef::StructFields;
        do {
          Token* mb_name = expect_ident();
          expect(":");
          nd->multiple.emplace_back(
              new NdKeyValuePair(*mb_name, new NdSymbol(*mb_name), ps_type_name()));
        } while (!is_end() && eat(","));
        expect(")");
        return nd;
      }

      auto type = ps_type_name();

      if (eat(",")) {
        nd->type = NdEnumeratorDef::MultipleTypes;
        nd->multiple.push_back(type);
        do {
          nd->multiple.emplace_back(ps_type_name());
        } while (!is_end() && eat(","));
        expect(")");
        return nd;
      }

      nd->type = NdEnumeratorDef::OneType;
      nd->variant = type;
      expect(")");
      return nd;
    }

    return nd;
  }

  NdEnum* Parser::ps_enum() {

    Token* tok = expect("enum");

    NdEnum* nd = new NdEnum(*tok);

    nd->name = *expect_ident();

    expect("{");

    if (eat("}")) {
      throw err::empty_class_or_enum_is_not_valid(*tok);
    }

    do {
      auto E = nd->enumerators.emplace_back(ps_enumerator_def());

      E->parent_enum_node = nd;
    } while (!is_end() && eat(","));

    expect("}");

    return nd;
  }

  NdNamespace* Parser::ps_namespace() {
    Token* tok = expect("namespace");

    std::string name {expect_ident()->text};

    NdNamespace* ns = new NdNamespace(*tok, name);

    Token* scope_tok = expect("{");

    if (eat("}"))
      return ns;

    while (!is_end()) {

      ns->items.emplace_back(ps_mod_item());

      if (eat("}"))
        return ns;
    }

    throw err::scope_not_terminated(*scope_tok);
  }

  Node* Parser::ps_mod_item() {
    if (look("var"))
      return ps_let();
    if (look("fn"))
      return ps_function();
    if (look("class"))
      return ps_class();
    if (look("enum"))
      return ps_enum();
    if (look("namespace"))
      return ps_namespace();

    throw err::expected_item_of_module(*cur);
  }

  void Parser::ps_do_import(Token* import_token, std::string path) {
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

  void Parser::ps_import() {

    auto import_token = cur;

    std::string path{expect_ident()->text};

    while (!is_end() && eat("::")) {
      path += "/";

      if (eat("*")) {
        todo;
      }

      path += expect_ident()->text;
    }

    expect(";");

    try {
      ps_do_import(import_token, std::filesystem::absolute(source.get_folder() + path).string());
    }
    catch (err::parses::cannot_open_file& e) {
      auto fol = FileSystem::GetFolderOfFile(source.path);

      while (true) {
        if (fol == Driver::get_instance()->get_first_cwd()) {
          e.msg.pop_back();
          e.msg += ".fire'";
          throw e;
        }

        fol = FileSystem::GetFolderOfFile(fol);

        if (auto tmp = fol + "/" + path; FileSystem::IsFile(tmp + ".fire")) {
          ps_do_import(import_token, tmp + ".fire");
          return;
        } else if (FileSystem::IsDirectory(tmp)) {
          ps_do_import(import_token, tmp);
          return;
        }
      }

      throw e;
    }
  }

  NdModule* Parser::ps_mod() {
    NdModule* mod = new NdModule(*cur);

    while (!is_end() && eat("import")) {
      ps_import();
    }

    for (auto&& src : source.imports) {
      if (src->is_node_imported)
        continue;

      auto submod = src->parse();

      for (auto&& item : submod->items) {
        mod->items.emplace_back(item);
      }

      src->is_node_imported = true;
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

  void Parser::merge_namespaces(std::vector<Node*>& items) {
    bool flag = false;

  __begin__:;
    flag = false;
    for (size_t i = 0; i < items.size();) {
      if (auto orig = items[i]->as<NdNamespace>(); orig->is(NodeKind::Namespace)) {
        for (size_t j = i + 1; j < items.size(); j++) {
          if (auto dup = items[j]->as<NdNamespace>();
              dup->is(NodeKind::Namespace) && dup->name == orig->name) {
            for (auto x : dup->items)
              orig->items.push_back(x);
            delete dup;
            items.erase(items.begin() + j);
            flag = true;
            goto __merged;
          }
        }
        i++;
      __merged:;
      } else {
        i++;
      }
    }

    if (flag)
      goto __begin__;

    for (auto&& x : items) {
      if (x->is(NodeKind::Namespace))
        merge_namespaces(x->as<NdNamespace>()->items);
    }
  }

  void Parser::reorder_items(std::vector<Node*>& items) {
    // move all enums at first

    std::vector<Node*> _new;

    for (auto&& x : items)
      if (x->is(NodeKind::Let)) {
        _new.push_back(x);
      }

    for (auto&& x : items)
      if (x->is(NodeKind::Namespace)) {
        _new.push_back(x);
      }

    for (auto&& x : items)
      if (x->is(NodeKind::Enum)) {
        _new.push_back(x);
      }

    for (auto&& x : items)
      if (x->is(NodeKind::Class)) {
        _new.push_back(x);
      }

    for (auto&& x : items)
      if (x->is(NodeKind::Function)) {
        _new.push_back(x);
      }

    items = std::move(_new);

    for (auto&& x : items) {
      if (x->is(NodeKind::Namespace))
        reorder_items(x->as<NdNamespace>()->items);
      else if (x->is(NodeKind::Module))
        reorder_items(x->as<NdModule>()->items);
    }
  }

  NdModule* Parser::parse() {
    auto mod = ps_mod();

    merge_namespaces(mod->items);

    reorder_items(mod->items);

    return mod;
  }

} // namespace fire