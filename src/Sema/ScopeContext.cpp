#include "macro.h"
#include "Sema.hpp"

namespace superman::sema {

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
      args.append(Symbol::new_arg_symbol(&arg))->var_info->is_type_deducted = true;
    }

    body = new UnnamedScope(func->body, func->args.size(), this);

    body->parent = this;
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
        todoimpl;
      }

      default:
        todoimpl;
      }
    }
  }

} // namespace superman::sema