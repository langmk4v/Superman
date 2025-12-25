#include <unordered_map>

#include "Sema.hpp"
#include "Utils.hpp"
#include "TypeInfo.hpp"
#include "VM.hpp"
#include "Error.hpp"

namespace fire {

  //
  // UnnamedScope
  //
  UnnamedScope::UnnamedScope(NdScope* scope, int var_offs, FunctionScope* fs) : ScopeContext(scope) {
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
      method_new->fnscope->is_method = true;
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
  TypeInfo Sema::make_class_type(NdClass* node) {
    TypeInfo ti{TypeKind::Class};
    ti.class_node = node;
    return ti;
  }

  //
  // new_var_symbol
  //
  Symbol* Symbol::new_var_symbol(NdLet* let, bool is_field) {
    auto s = new Symbol(SymbolKind::Var, let->name.text, let);

    s->var_info = new VariableInfo();
    s->var_info->def_let = let;
    s->var_info->is_field = is_field;

    let->var_info_ptr = s->var_info;

    return s;
  }

  //
  // new_arg_symbol
  //
  Symbol* Symbol::new_arg_symbol(NdFunction::Argument* arg) {
    auto s = new Symbol(SymbolKind::Var, arg->name.text, arg);

    s->var_info = new VariableInfo();
    s->var_info->def_arg = arg;

    arg->var_info_ptr = s->var_info;

    return s;
  }

  //
  // find_symbol
  //
  SymbolFindResult Sema::find_symbol(NdSymbol* node) {
    SymbolFindResult result = SymbolFindResult();

    // find first symbol ( A of A::B::... )
    for (ScopeContext* scope = cur_scope; scope; scope = scope->parent) {
      if (scope->find_symbol(result.matches, node->name.text) >= 1) {
        break;
      }
    }

    if (result.empty()) {
      // find in builtin functions

      size_t const count = std::size(builtin_func_table);

      for (size_t i = 0; i < count; i++) {
        auto fn = builtin_func_table[i];

        if (fn->name == node->name.text) {
          result.builtin_f = fn;
          goto _found_builtin;
        }
      }

      return result;
    }

  _found_builtin:;

    // loop for scope resolutions
    for (node = node->next; node; node = node->next) {

      std::string const& name = node->name.text;

      printd(name);

      todo;
    }

    return result;
  }

  //
  // get_required_template_params_count
  //
  int Sema::get_required_template_params_count(Symbol* s) {
    if (s->is_type_name()) {
      if (s->is(SymbolKind::Enum)) {
        todoimpl;
      }
      if (s->is(SymbolKind::Class)) {
        todoimpl;
      }
    }

    if (s->is(SymbolKind::Func)) {
      todoimpl;
    }

    if (s->is(SymbolKind::Var)) {
      // variable is still not template yet... ?
      todoimpl;
    }

    return 0; // no needed.
  }

  void Sema::analyze_all(NdModule* mod) {
    auto se = Sema(mod);

    se.analyze_full();
  }

  void Sema::analyze_full() { check_module(root_scope); }

  void Sema::check_module(ModuleScope* mod) {

    auto node = mod->node->as<NdModule>();

    auto _save = cur_scope;
    cur_scope = mod;

    for (auto item : node->items) {
      switch (item->kind) {
      case NodeKind::Let: {
        check_let(item->as<NdLet>());
        break;
      }

      case NodeKind::Function: {
        check_function(item->scope_ptr->as<FunctionScope>());
        break;
      }

      case NodeKind::Class: {
        check_class(item->scope_ptr->as<ClassScope>());
        break;
      }

      default:
        todoimpl;
      }
    }

    cur_scope = _save;
  }

  void Sema::check_class(ClassScope* cs) {
    NdClass* node = cs->node->as<NdClass>();

    (void)node;

    for (auto f : cs->fields)
      check_let(f->node->as<NdLet>());

    for (auto m : cs->methods)
      check_function(m->fnscope);

    if (cs->method_new) check_function(cs->method_new->fnscope);
    if (cs->method_delete) check_function(cs->method_delete->fnscope);
  }

  void Sema::check_function(FunctionScope* func) {
    NdFunction* node = func->node->as<NdFunction>();

    assert(node->scope_ptr == func);

    std::unordered_map<std::string, Token*> arg_dup_check;

    // check argument name duplications
    for (Symbol* arg : func->args) {
      if (auto it = arg_dup_check.find(arg->name); it != arg_dup_check.end()) {
        throw err::duplicate_of_definition(arg->var_info->def_arg->name, *it->second);
      } else {
        arg_dup_check[arg->name] = &arg->var_info->def_arg->name;
      }
    }

    // check argument types
    for (Symbol* arg : func->args) {
      arg->var_info->type = eval_typename(arg->var_info->def_arg->type).type;
      arg->var_info->is_type_deducted = true;
    }

    // get function result type
    if (node->result_type) {
      func->result_type = eval_typename(node->result_type).type;
      func->is_result_type_specified = true;
    }

    auto _save = cur_scope;
    cur_scope = func;

    // check body scope
    check_scope(func->body);

    cur_scope = _save;
  }

  void Sema::check_scope(UnnamedScope* scope) {
    auto node = scope->node->as<NdScope>();

    auto _save = cur_scope;
    cur_scope = scope;

    for (auto stmt : node->items) {
      switch (stmt->kind) {
      case NodeKind::Scope:
        check_scope(stmt->scope_ptr->as<UnnamedScope>());
        break;

      case NodeKind::Let: {
        check_let(stmt->as<NdLet>());
        break;
      }

      case NodeKind::Return: {
        auto ret = stmt->as<NdReturn>();

        auto fs = get_cur_func_scope();
        assert(fs);

        // 戻り値なし ( "return" 単体 )
        if (!ret->expr) {
          // => 関数定義側で指定されている場合はエラー
          //    ( return 文に式が必要 )
          if (fs->is_result_type_specified) {
            err::mismatched_return_statement(ret->token).print();
            warns::show_note(fs->node->as<NdFunction>()->result_type->token,
                             "function result type was specified here, so return-statement must "
                             "have an expression.")();
          }
        }
        // 戻り値あり
        else {
          // => 定義側で指定されていない場合はエラー
          if (!fs->is_result_type_specified) {
            err::mismatched_return_statement(ret->token).print();
            warns::show_note(fs->node->as<NdFunction>()->token,
                             "function result type is not specified, but return-statement has an expression.")();
          }

          // => 指定ありで型が一致しない場合はエラー
          if (auto ex = eval_expr(ret->expr).type; !fs->result_type.equals(ex)) {
            err::mismatched_types(ret->expr->token, fs->result_type.to_string(), ex.to_string()).print();
            warns::show_note(fs->node->as<NdFunction>()->result_type->token, "specified here")();
          }
        }

        break;
      }

      default:
        eval_expr(stmt);
        break;
      }
    }

    cur_scope = _save;
  }

  //
  // check_let
  //
  void Sema::check_let(NdLet* let) {
    debug(assert(let->var_info_ptr));

    auto& varinfo = *let->var_info_ptr;

    // 型指定あり
    if (let->type) {
      varinfo.type = eval_typename(let->type).type;
      varinfo.is_type_deducted = true;
    }

    // 初期化式あり
    if (let->init) {
      auto result = eval_expr(let->init);

      if (result.is_succeed) {
        // 型が既にわかっている場合
        if (varinfo.is_type_deducted) {
          // => それと一致しなければエラー
          if (!varinfo.type.equals(result.type)) {
            throw err::mismatched_types(let->init->token, varinfo.type.to_string(), result.type.to_string());
          }
        }
        // 型がまだ不明
        else {
          // => ここで決定
          varinfo.is_type_deducted = true;
          varinfo.type = result.type;
        }
      }
    }
  }

  Sema::Sema(NdModule* mod) { root_scope = new ModuleScope(mod); }

} // namespace fire