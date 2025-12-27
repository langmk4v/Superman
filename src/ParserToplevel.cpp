#include <filesystem>

#include "Driver.hpp"
#include "Error.hpp"
#include "FSHelper.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "strconv.hpp"

namespace fire {

NdFunction* Parser::ps_function(bool is_method) {
  auto& tok = *expect("fn");
  auto node = new NdFunction(tok, *expect_ident());
  _parse_template_param_defs(*node);
  if (eat("(")) {
    if (!eat(")")) {
      if (is_method && eat("self")) {
        node->take_self = true;
        if (!eat_comma())
          goto L_fn_pass_args;
      }
      do {
        auto arg_name = expect_ident();
        expect_colon();
        auto arg_type = ps_type_name();
        node->args.emplace_back(new NdFuncArgument(*arg_name, arg_type));
      } while (!is_end() && eat_comma());
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

  if (eat_colon())
    node->base_class = ps_type_name();

  expect_curly_open();

  if (eat_curly_close()) {
    throw err::empty_class_or_enum_is_not_valid(tok);
  }

  while (!is_end()) {
    auto public_flag = eat("pub");
    // method
    if (look("fn")) {
      auto M = node->methods.emplace_back(ps_function(true));
      M->is_pub = public_flag;
      M->take_self = true;
    }
    // field
    else if (look("var")) {
      node->fields.emplace_back(ps_let())->is_pub = public_flag;
    } else {
      expect("}");
      return node;
    }
  }

  throw err::scope_not_terminated(tok);
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
        expect_colon();
        nd->multiple.emplace_back(new NdKeyValuePair(
            *mb_name, new NdSymbol(*mb_name), ps_type_name()));
      } while (!is_end() && eat_comma());
      expect(")");
      return nd;
    }
    auto type = ps_type_name();
    if (eat_comma()) {
      nd->type = NdEnumeratorDef::MultipleTypes;
      nd->multiple.push_back(type);
      do {
        nd->multiple.emplace_back(ps_type_name());
      } while (!is_end() && eat_comma());
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
  } while (!is_end() && eat_comma());
  expect("}");
  return nd;
}

NdNamespace* Parser::ps_namespace() {
  Token* tok = expect("namespace");
  std::string name{expect_ident()->text};
  NdNamespace* ns = new NdNamespace(*tok, name);
  Token* scope_tok = expect("{");
  if (eat("}"))
    return ns;
  while (!is_end()) {
    ns->items.emplace_back(ps_toplevel());
    if (eat("}"))
      return ns;
  }
  throw err::scope_not_terminated(*scope_tok);
}

Node* Parser::ps_toplevel() {
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
    auto item = mod->items.emplace_back(ps_toplevel());
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

} // namespace fire