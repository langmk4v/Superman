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