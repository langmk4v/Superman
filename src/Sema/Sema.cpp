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

          if (item->is(NodeKind::Let)) {
            auto s = mod_scope->variables.append(new_variable_symbol(item->as<NdLet>()));
            mod_scope->symtable.append(s);
            continue;
          }

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

          if (item->is(NodeKind::Let)) {
            auto s = ns_scope->variables.append(new_variable_symbol(item->as<NdLet>()));
            ns_scope->symtable.append(s);
            continue;
          }

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

          auto en_sym = new Symbol();
          en_sym->name = enumerator->name.text;
          en_sym->kind = SymbolKind::Enumerator;
          en_sym->node = enumerator;
          en_sym->scope = nullptr;

          enu_scope->symtable.append(en_sym);
          enu_scope->enumerators.append(en_sym);
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

          fn_scope->symtable.append(arg_symbol);
          fn_scope->arguments.append(arg_symbol);
        }

        fn_scope->body = create_scope(fn->body, fn_scope)->as<SCScope>();

        return fn_scope;
      }

      case NodeKind::Try: {
        auto try_nd = node->as<NdTry>();

        auto try_scope = new SCTry(try_nd, parent);

        try_nd->scope_ptr = try_scope;

        try_scope->body = create_scope(try_nd->body, try_scope)->as<SCScope>();

        for (auto& catch_nd : try_nd->catches) {
          auto catch_scope = new SCCatch(catch_nd, try_scope);

          catch_scope->holder_name = new_variable_symbol(&catch_nd->holder, catch_nd->holder.text);
          catch_scope->symtable.append(catch_scope->holder_name);

          catch_scope->body = create_scope(catch_nd->body, catch_scope)->as<SCScope>();

          try_scope->catches.push_back(catch_scope);
        }

        if (try_nd->finally_block) {
          auto finally_scope = new SCScope(try_nd->finally_block, try_scope);
          try_scope->finally_scope = finally_scope;
        }

        return try_scope;
      }

      case NodeKind::Scope: {
        auto sc = node->as<NdScope>();
        auto scope = new SCScope(sc, parent);

        sc->scope_ptr = scope;

        for (auto& item : sc->items) {
          switch (item->kind) {
            case NodeKind::Let: {
              auto s = scope->variables.append(new_variable_symbol(item->as<NdLet>()));
              scope->symtable.append(s);
              break;
            }

            case NodeKind::Scope:
            case NodeKind::For:
            case NodeKind::If:
              scope->subscopes.push_back(create_scope(item, scope));
              break;
          }
        }

        return scope;
      }

      case NodeKind::If: {
        auto if_nd = node->as<NdIf>();
        auto if_scope = new SCIf(if_nd, parent);

        if_nd->scope_ptr = if_scope;

        if (if_nd->vardef) {
          if_scope->var = new_variable_symbol(if_nd->vardef);
          if_scope->symtable.append(if_scope->var);
        }

        if_scope->then_scope = create_scope(if_nd->thencode, if_scope)->as<SCScope>();

        if (if_nd->elsecode) {
          if_scope->else_scope = create_scope(if_nd->elsecode, if_scope)->as<SCScope>();
        }

        return if_scope;
      }

      case NodeKind::For: {
        auto for_nd = node->as<NdFor>();
        auto for_scope = new SCFor(for_nd, parent);

        for_nd->scope_ptr = for_scope;

        for_scope->iter_name = new_variable_symbol(&for_nd->iter, for_nd->iter.text);

        for_scope->symtable.append(for_scope->iter_name);

        for_scope->body = create_scope(for_nd->body, for_scope)->as<SCScope>();

        return for_scope;
      }
    }

    printd(static_cast<int>(node->kind));
    todo;
  }

  NdVisitorContext Sema::resolve_names(Node* node, NdVisitorContext ctx) {
    ctx.node = node;

    switch (node->kind) {
      case NodeKind::Symbol: {
        auto sym = node->as<NdSymbol>();

        auto result = find_symbol(sym, ctx);

        if (result.hits.empty()) {
          if (result.previous)
            throw err::use_of_undefined_symbol(sym->token, result.previous->name.text,
                                               result.node->name.text);

          throw err::use_of_undefined_symbol(sym->token);
        }

        if (result.hits.size() >= 2) { throw err::ambiguous_symbol_name(sym->token); }

        sym->symbol_ptr = result.hits[0];

        break;
      }

      case NodeKind::Array: {
        auto arr = node->as<NdArray>();

        for (auto& item : arr->data)
          resolve_names(item, ctx);

        break;
      }

      case NodeKind::Tuple: {
        auto tuple = node->as<NdTuple>();

        for (auto& item : tuple->elems)
          resolve_names(item, ctx);

        break;
      }

      case NodeKind::Slice: {
        auto slice = node->as<NdExpr>();
        if (slice->lhs) resolve_names(slice->lhs, ctx);
        if (slice->rhs) resolve_names(slice->rhs, ctx);
        break;
      }

      case NodeKind::Subscript: {
        auto ss = node->as<NdExpr>();
        if (ss->lhs) resolve_names(ss->lhs, ctx);
        if (ss->rhs) resolve_names(ss->rhs, ctx);
        break;
      }

      case NodeKind::MemberAccess: {
        auto ma = node->as<NdExpr>();

        resolve_names(ma->lhs, ctx);

        // Don't check right side in this process.
        // (because needed type-info of left side)

        break;
      }

      case NodeKind::CallFunc: {
        auto cf = node->as<NdCallFunc>();

        resolve_names(cf->callee, ctx);

        for (auto& arg : cf->args)
          resolve_names(arg, ctx);

        break;
      }

      case NodeKind::Scope: {
        auto sc = node->as<NdScope>();

        assert(sc->scope_ptr);

        ctx.cur_scope = sc->scope_ptr;

        for (auto& item : sc->items)
          resolve_names(item, ctx);

        break;
      }

      case NodeKind::Let: {
        auto let = node->as<NdLet>();

        if (let->init) resolve_names(let->init, ctx);

        break;
      }

      case NodeKind::If: {
        auto if_node = node->as<NdIf>();

        ctx.cur_scope = if_node->scope_ptr;
        resolve_names(if_node->cond, ctx);

        resolve_names(if_node->thencode, ctx);

        if (if_node->elsecode) resolve_names(if_node->elsecode, ctx);

        break;
      }

      case NodeKind::For: {
        auto for_node = node->as<NdFor>();
        auto for_scope = for_node->scope_ptr->as<SCFor>();

        ctx.cur_scope = for_scope;

        resolve_names(for_node->iterable, ctx);
        resolve_names(for_node->body, ctx);

        break;
      }

      case NodeKind::Try: {
        auto try_nd = node->as<NdTry>();
        auto try_scope = try_nd->scope_ptr->as<SCTry>();

        ctx.cur_scope = try_scope;
        resolve_names(try_nd->body, ctx);

        for (auto& catch_nd : try_nd->catches) {
          ctx.cur_scope = catch_nd->scope_ptr;
          resolve_names(catch_nd, ctx);
        }

        if (try_nd->finally_block) {
          ctx.cur_scope = try_scope->finally_scope;
          resolve_names(try_nd->finally_block, ctx);
        }

        break;
      }

      case NodeKind::Function: {
        auto fn = node->as<NdFunction>();
        auto fn_scope = fn->scope_ptr->as<SCFunction>();

        resolve_names(fn->body, {.cur_scope = fn_scope->body, .cur_func = fn_scope});

        break;
      }

      case NodeKind::Module: {
        auto mod = node->as<NdModule>();

        for (auto& item : mod->items)
          resolve_names(item, {.cur_scope = item->scope_ptr});

        break;
      }
    }

    return ctx;
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

    return symbol;
  }

  Symbol* Sema::new_variable_symbol(Token* tok, std::string const& name) {
    auto symbol = new Symbol();

    symbol->name = name;
    symbol->kind = SymbolKind::Var;
    symbol->token = tok;

    symbol->var_info = new VariableInfo();

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

  SymbolFindResult Sema::find_symbol(NdSymbol* node, NdVisitorContext ctx) {
    (void)node;
    (void)ctx;

    SymbolFindResult result = {.node = node};

    for (auto scope = ctx.cur_scope; scope; scope = scope->parent) {
      for (auto& symbol : scope->symtable.symbols) {
        if (symbol->name == node->name.text) { result.hits.push_back(symbol); }
      }

      if (result.hits.size() >= 1) break;
    }

    if (result.hits.empty()) return result;

    if (node->next) {

      if (result.hits.size() >= 2) { throw err::ambiguous_symbol_name(node->token); }

      ctx.cur_scope = result.hits[0]->scope;

      switch (ctx.cur_scope->kind) {
        case ScopeKind::Namespace:
        case ScopeKind::Class:
        case ScopeKind::Enum: {
          result = find_symbol(node->next, ctx);
          result.previous = node;
          return result;
        }
      }

      throw err::invalid_scope_resolution(*node->scope_resol_tok);
    }

    return result;
  }

} // namespace fire