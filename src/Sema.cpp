#include <iostream>

#include "Utils.hpp"
#include "Error.hpp"

#include "Sema/Sema.hpp"

namespace fire {

  Sema::Sema() {
  }

  void Sema::analyze_all(NdModule* mod) {
    auto se = Sema();
    se.analyze_full(mod);
  }

  void Sema::analyze_full(NdModule* mod) {
    root_scope = create_scope(mod, nullptr)->as<SCModule>();

    resolve_names(mod, {.cur_scope = root_scope});

    infer_types(mod);

    check_semantics(mod);
  }

  Scope* Sema::create_scope(Node* node, Scope* parent) {
    switch (node->kind) {
      case NodeKind::Module: {
        auto mod = node->as<NdModule>();
        auto mod_scope = new SCModule(mod, parent);

        mod->scope_ptr = mod_scope;

        mod_scope->symbol = {
            .name = mod->name,
            .kind = SymbolKind::Module,
            .node = mod,
            .scope = mod_scope,
        };

        for (auto& item : mod->items) {
          auto scope = create_scope(item, mod_scope);

          mod_scope->symtable.append(&scope->symbol);

          mod_scope->get_table(item->kind)->append(&scope->symbol);
        }

        return mod_scope;
      }

      case NodeKind::Namespace: {
        auto ns = node->as<NdNamespace>();
        auto ns_scope = new SCNamespace(ns, parent);

        ns->scope_ptr = ns_scope;

        ns_scope->symbol = {
            .name = ns->name,
            .kind = SymbolKind::Namespace,
            .node = ns,
            .scope = ns_scope,
        };

        for (auto& item : ns->items) {
          auto scope = create_scope(item, ns_scope);

          ns_scope->symtable.append(&scope->symbol);

          ns_scope->get_table(item->kind)->append(&scope->symbol);
        }

        return ns_scope;
      }

      case NodeKind::Class: {
        auto cla = node->as<NdClass>();
        auto cla_scope = new SCClass(cla, parent);

        cla->scope_ptr = cla_scope;

        cla_scope->symbol = {
            .name = cla->name.text,
            .kind = SymbolKind::Class,
            .node = cla,
            .scope = cla_scope,
        };

        for (auto& field : cla->fields) {
          cla_scope->fields.append(new_variable_symbol(field));
        }

        for (auto& method : cla->methods) {
          auto scope = create_scope(method, cla_scope);
          cla_scope->symtable.append(&scope->symbol);
          cla_scope->methods.append(&scope->symbol);
        }

        return cla_scope;
      }

      case NodeKind::Enum: {
        auto enu = node->as<NdEnum>();
        auto enu_scope = new SCEnum(enu, parent);

        enu->scope_ptr = enu_scope;

        enu_scope->symbol = {
            .name = enu->name.text,
            .kind = SymbolKind::Enum,
            .node = enu,
            .scope = enu_scope,
        };

        for (auto& enumerator : enu->enumerators) {
          auto scope = create_scope(enumerator, enu_scope);
          enu_scope->symtable.append(&scope->symbol);
          enu_scope->enumerators.append(&scope->symbol);
        }

        return enu_scope;
      }

      case NodeKind::Function: {
        auto fn = node->as<NdFunction>();
        auto fn_scope = new SCFunction(fn, parent);

        fn->scope_ptr = fn_scope;

        fn_scope->symbol = {
            .name = fn->name.text,
            .kind = SymbolKind::Func,
            .node = fn,
            .scope = fn_scope,
        };

        for (auto& arg : fn->args) {
          auto arg_symbol = new Symbol();
          arg_symbol->name = arg.name.text;
          arg_symbol->kind = SymbolKind::Var;
          arg_symbol->node = &arg;
          fn_scope->arguments.append(arg_symbol);
        }

        fn_scope->body = create_scope(fn->body, fn_scope)->as<SCScope>();

        return fn_scope;
      }

      case NodeKind::Scope: {
        auto sc = node->as<NdScope>();
        auto scope = new SCScope(sc, parent);

        sc->scope_ptr = scope;

        for (auto& item : sc->items) {
          switch (item->kind) {
            case NodeKind::Let: {
              scope->variables.append(new_variable_symbol(item->as<NdLet>()));
              break;
            }
            case NodeKind::Scope: {
              scope->subscopes.push_back(create_scope(item, scope));
              break;
            }
          }
        }

        return scope;
      }
    }

    printd(static_cast<int>(node->kind));
    todo;
  }

  void Sema::resolve_names(Node* node, NdVisitorContext ctx) {
    (void)node;
    (void)ctx;

    switch (node->kind) {
      case NodeKind::Symbol: {
        auto sym = node->as<NdSymbol>();

        sym->symbol_ptr = find_symbol(sym, ctx);

        if (!sym->symbol_ptr) { throw err::use_of_undefined_symbol(sym->token); }

        break;
      }

      case NodeKind::Scope: {
        auto sc = node->as<NdScope>();
      }
    }
  }

  void Sema::infer_types(Node* node) {
    (void)node;
  }

  void Sema::check_semantics(Node* node) {
    (void)node;
  }

  Symbol* Sema::new_variable_symbol(NdLet* let) {
    auto symbol = new Symbol();

    symbol->name = let->name.text;
    symbol->kind = SymbolKind::Var;
    symbol->node = let;

    symbol->var_info = new VariableInfo();
    symbol->var_info->is_type_deducted = false;

    return symbol;
  }

  TypeInfo Sema::eval_expr_ty(Node* node, NdVisitorContext ctx) {
    (void)node;
    (void)ctx;
    todo;
  }

  TypeInfo Sema::eval_typename_ty(NdSymbol* node, NdVisitorContext ctx) {
    (void)node;
    (void)ctx;
    todo;
  }

  Symbol* Sema::find_symbol(NdSymbol* node, NdVisitorContext ctx) {
    (void)node;
    (void)ctx;
    todo;
  }

} // namespace fire