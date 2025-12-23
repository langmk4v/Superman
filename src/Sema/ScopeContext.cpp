#include "Utils/macro.h"
#include "Sema/Sema.hpp"

namespace fire::sema {

  //
  // UnnamedScope
  //
  UnnamedScope::UnnamedScope(NdScope* scope, int var_offs, FunctionScope* fs)
      : ScopeContext(scope) {
    scope->scope_ptr = this;

    parent_fn = fs;

    for (auto item : scope->items) {
      if (item->is(NodeKind::Let)) {
        auto let = item->as<NdLet>();
        auto var = variables.append(Symbol::new_var_symbol(let));

        var->var_info->offset = let->index = var_offs++;

        continue;
      }

      if (item->is(NodeKind::Scope)) {
        auto sub = add_scope(new UnnamedScope(item->as<NdScope>(), var_offs, fs));
        sub->parent = this;
        continue;
      }
    }

    fs->local_var_count = std::max<int>(fs->local_var_count, var_offs);
  }

  //
  // FunctionScope
  //
  FunctionScope::FunctionScope(NdFunction* func) : ScopeContext(func) {
    func->scope_ptr = this;

    for (auto& arg : func->args) {
      args.append(Symbol::new_arg_symbol(&arg));
    }

    body = new UnnamedScope(func->body, func->args.size(), this);

    body->parent = this;
  }

  //
  // ClassScope
  //
  ClassScope::ClassScope(NdClass* cla) : ScopeContext(cla) {
    cla->scope_ptr = this;

    for (auto f : cla->fields)
      fields.append(Symbol::new_var_symbol(f, true));

    for (auto m : cla->methods) {
      auto ms = methods.append(new Symbol(SymbolKind::Method, m->name.text, m));

      auto ctx = new FunctionScope(m);
      ctx->parent = this;
      ctx->is_method = true;

      ms->scope_ctx = ctx;
    }

    if (cla->m_new) {
      method_new = new Symbol(SymbolKind::Func, "new", cla->m_new);
      method_new->fnscope = new FunctionScope(cla->m_new);
      method_new->fnscope->parent = this;
    }

    if (cla->m_delete) {
      todoimpl;
    }
  }

  //
  // ModuleScope
  //
  ModuleScope::ModuleScope(NdModule* mod) : ScopeContext(mod) {

    mod->scope_ptr = this;

    int global_var_offs = 0;

    for (auto item : mod->items) {
      switch (item->kind) {
      case NodeKind::Let: {
        auto let = item->as<NdLet>();
        auto gv = variables.append(Symbol::new_var_symbol(let));

        gv->var_info->is_global = true;
        gv->var_info->offset = global_var_offs++;

        let->index = gv->var_info->offset;

        break;
      }

      case NodeKind::Function: {
        auto func = item->as<NdFunction>();

        auto ctx = new FunctionScope(func);
        ctx->parent = this;

        auto fsym = functions.append(new Symbol(SymbolKind::Func, func->name.text, func));
        fsym->scope_ctx = ctx;

        break;
      }

      case NodeKind::Enum: {
        todoimpl;
      }

      case NodeKind::Class: {
        auto cla = item->as<NdClass>();

        auto ctx = new ClassScope(cla);
        ctx->parent = this;

        auto cs = new Symbol(SymbolKind::Class, cla->name.text, cla);
        cs->scope_ctx = ctx;

        classes.append(cs);

        break;
      }

      default:
        todoimpl;
      }
    }
  }

} // namespace superman::sema