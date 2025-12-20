#include <unordered_map>

#include "macro.h"
#include "Strings.hpp"
#include "Object.hpp"
#include "Node.hpp"
#include "Error.hpp"
#include "Sema.hpp"

/*
 * ＠自分へ
 *  ソースコードみてわけわかんないときはコンサータ飲んでください。
 *  また途中放棄とかはしないでください。
 *  宜しくお願いします。
 */

namespace superman::sema {

  //
  // new_var_symbol
  //  ローカル変数またはグローバル変数用のシンボル情報を作成
  //
  Symbol* Symbol::new_var_symbol(NdLet* let) {
    auto s = new Symbol(SymbolKind::Var, let->name.text, let);

    s->var_info = new VariableInfo();
    s->var_info->def_let = let;

    let->var_info_ptr = s->var_info;

    return s;
  }

  //
  // new_arg_symbol
  //  関数の引数用のシンボル情報を作成
  //
  Symbol* Symbol::new_arg_symbol(NdFunction::Argument* arg) {
    auto s = new Symbol(SymbolKind::Var, arg->name.text, arg);

    s->var_info = new VariableInfo();
    s->var_info->def_arg = arg;

    arg->var_info_ptr = s->var_info;

    return s;
  }

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

    body = new UnnamedScope(func->body, 0, this);

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

  //
  // eval_expr
  //
  ExprTypeResult Sema::eval_expr(Node* node) {
    switch (node->kind) {
    case NodeKind::Value:
      return node->as<NdValue>()->obj->type;

    case NodeKind::Symbol: {
      auto sym = node->as<NdSymbol>();

      auto result = find_symbol(sym);

      if (result.empty() && result.blt_funcs.empty()) throw err::use_of_undefined_symbol(sym->name);

      if (result.count() + result.blt_funcs.size() >= 2) {
        throw err::ambiguous_symbol_name(sym->name);
      }

      if (result.count()) {
        auto S = result.matches[0];

        sym->sym_target = S->node;

        switch (S->kind) {
        case SymbolKind::Var: {
          if (!S->var_info->is_type_deducted) {
            todoimpl;
          }

          sym->type = NdSymbol::Var;
          sym->is_global_var = S->var_info->is_global;
          sym->var_offset = S->var_info->offset;

          return S->var_info->type;
        }

        default:
          todoimpl;
        }
      }

      sym->type = NdSymbol::BuiltinFunc;
      sym->sym_target_bltin = result.blt_funcs[0];

      auto ti = TypeInfo(TypeKind::Function, sym->sym_target_bltin->arg_types, false, false);

      ti.parameters.insert(ti.parameters.begin(), sym->sym_target_bltin->result_type);

      auto res = ExprTypeResult(ti);
      res.builtin_func = sym->sym_target_bltin;

      return res;
    }

    case NodeKind::CallFunc: {
      auto cf = node->as<NdCallFunc>();

      auto callee = eval_expr(cf->callee);

      if (callee.fail()) {
        todoimpl;
      }

      if (callee.type.kind != TypeKind::Function) {
        throw err::not_callable_type(cf->callee->token, callee.type.to_string());
      }

      bool is_blt = callee.builtin_func != nullptr;

      cf->blt_fn = callee.builtin_func;

      auto calls = (int)cf->args.size();
      auto takes = (int)callee.type.parameters.size() - 1;

      if (calls != takes) {
        bool is_var_arg = is_blt && callee.builtin_func->is_variable_args;

        if (!is_var_arg) {
          // todo: get isvararg flag of user-def func
        }

        if (is_var_arg ? (calls < takes - 1) : (calls < takes)) {
          todoimpl; // too few
        } else if (!is_var_arg && calls > takes) {
          todoimpl; // too many
        }
      }

      for (int i = 0; i < std::min(calls, takes); i++) {
        auto argtype = eval_expr(cf->args[i]).type;

        if (!argtype.equals(is_blt ? callee.builtin_func->arg_types[i] : TypeInfo(/*todo*/))) {
          todoimpl;
        }
      }

      for (int i = takes; i < calls; i++) {
        eval_expr(cf->args[i]);
      }

      if (is_blt) return callee.builtin_func->result_type;

      todoimpl;
    }
    }

    todoimpl;
  }

  //
  // eval_typename
  //
  ExprTypeResult Sema::eval_typename(NdSymbol* node) {
    std::pair<char const*, TypeKind> name_kind_pairs[]{
        {"none", TypeKind::None},         {"int", TypeKind::Int},     {"float", TypeKind::Float},
        {"bool", TypeKind::Bool},         {"char", TypeKind::Char},   {"string", TypeKind::String},
        {"vector", TypeKind::Vector},     {"tuple", TypeKind::Tuple}, {"dict", TypeKind::Dict},
        {"function", TypeKind::Function},
    };

    auto result = ExprTypeResult();

    // 基本型の名前から探す
    for (auto&& [s, k] : name_kind_pairs)
      if (node->name.text == s) {
        result.type.kind = k;

        // テンプレート引数が多すぎるまたは少なすぎる
        if (int C = TypeInfo::required_param_count(k), N = (int)node->te_args.size(); C != N)
          throw err::no_match_template_arguments(node->name, C, N);

        // 存在したらユーザー定義型の検索はスキップ
        goto _Skip_Find_Userdef;
      }

    {
      auto found = find_symbol(node);

      if (found.empty()) throw err::unknown_type_name(node->name);

      for (size_t i = 0; i < found.count();) {
        if (!found[i]->is_type_name()) {
          found.remove(i);
          continue;
        }
        i++;
      }

      if (found.count() >= 2) {
        throw err::ambiguous_symbol_name(node->name);
      }

      auto Final = found[0];
      (void)Final;

      todoimpl;
    }
  _Skip_Find_Userdef:;

    return result;
  }

  //
  // find_symbol
  //
  SymbolFindResult Sema::find_symbol(NdSymbol* node) {
    auto r = SymbolFindResult();

    (void)node;

    // find first symbol ( A of A::B::... )
    for (auto scope = cur_scope; scope; scope = scope->parent) {
      if (scope->find_symbol(r.matches, node->name.text) >= 1) {
        break;
      }
    }

    if (r.empty()) {
      // find in builtin functions
      for (auto&& bf : builtins::Function::all_builtin_funcs)
        if (bf->name == node->name.text) r.blt_funcs.push_back(bf);

      if (r.blt_funcs.empty()) return r;
    }

    // loop for scope resolutions
    for (node = node->next; node; node = node->next) {
      todoimpl;
    }

    return r;
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
      }
    }

    cur_scope = _save;
  }

  void Sema::check_function(FunctionScope* func) {
    auto node = func->node->as<NdFunction>();

    std::unordered_map<std::string, Token*> arg_dup_check;

    // check argument name duplications
    for (auto& arg : func->args) {
      if (auto it = arg_dup_check.find(arg->name); it != arg_dup_check.end()) {
        throw err::duplicate_of_definition(arg->var_info->def_arg->name, *it->second);
      } else {
        arg_dup_check[arg->name] = &arg->var_info->def_arg->name;
      }
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
            warns::show_note(
                fs->node->as<NdFunction>()->token,
                "function result type is not specified, but return-statement has an expression.")();
          }

          // => 指定ありで型が一致しない場合はエラー
          if (auto ex = eval_expr(ret->expr).type; !fs->result_type.equals(ex)) {
            err::mismatched_types(ret->expr->token, fs->result_type.to_string(), ex.to_string())
                .print();
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
            throw err::mismatched_types(let->init->token, varinfo.type.to_string(),
                                        result.type.to_string());
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

} // namespace superman::sema