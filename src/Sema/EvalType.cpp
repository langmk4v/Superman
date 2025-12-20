#include <algorithm>

#include "macro.h"
#include "Error.hpp"
#include "Sema.hpp"

namespace superman::sema {

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
          if (!S->var_info->is_argument() && !S->var_info->is_type_deducted) {
            todoimpl;
          }

          debug(
          if(S->var_info->is_argument()) {
            assert(S->var_info->is_type_deducted);
          })

          sym->type = NdSymbol::Var;
          sym->is_global_var = S->var_info->is_global;
          sym->var_offset = S->var_info->offset;

          return S->var_info->type;
        }

        case SymbolKind::Func: {
          sym->type = NdSymbol::Func;

          auto f = S->node->as<NdFunction>();

          assert(f->is(NodeKind::Function));

          auto ti = TypeInfo(TypeKind::Function,
                             {f->result_type ? eval_typename(f->result_type).type : TypeKind::None},
                             false, false);

          for (auto&& arg : f->args)
            ti.parameters.emplace_back(eval_typename(arg.type).type);

          auto res = ExprTypeResult(ti);

          res.func_nd = f;

          return res;
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

      if (callee.builtin_func)
        cf->blt_fn = callee.builtin_func;
      else if (callee.func_nd)
        cf->func_nd = callee.func_nd;

      auto calls = (int)cf->args.size();
      auto takes = (int)callee.type.parameters.size() - 1;

      if (calls != takes) {
        bool is_var_arg = is_blt && callee.builtin_func->is_variable_args;

        if (!is_var_arg) {
          // todo: get isvararg flag of user-def func
        }

        if (is_var_arg ? (calls < takes - 1) : (calls < takes)) {
          throw err::too_few_arguments(cf->callee->token);
        } else if (!is_var_arg && calls > takes) {
          throw err::too_many_arguments(cf->callee->token);
        }
      }

      for (int i = 0; i < std::min(calls, takes); i++) {
        auto argtype = eval_expr(cf->args[i]).type;

        // if (!argtype.equals(is_blt ? callee.builtin_func->arg_types[i] : TypeInfo(/*todo*/))) {
        if (!argtype.equals(callee.type.parameters[i + 1])) {
          todoimpl;
        }
      }

      for (int i = takes; i < calls; i++) {
        eval_expr(cf->args[i]);
      }

      return callee.type.parameters[0];
    }

    case NodeKind::Assign: {
      todoimpl;
    }
    }

    if (!node->is_expr()) {
      todoimpl;
    }

    auto ex = node->as<NdExpr>();

    auto lhs_type = eval_expr(ex->lhs).type;
    auto rhs_type = eval_expr(ex->rhs).type;

    struct TypeRule {
      TypeKind l, r;
      char const* op;
      TypeKind res;
    };

    static TypeRule const type_rules[] = {
        {TypeKind::Int, TypeKind::Int, "+", TypeKind::Int},
        {TypeKind::Int, TypeKind::Int, "-", TypeKind::Int},
        {TypeKind::Int, TypeKind::Int, "*", TypeKind::Int},
        {TypeKind::Int, TypeKind::Int, "/", TypeKind::Int},
        {TypeKind::Float, TypeKind::Float, "+", TypeKind::Float},
        {TypeKind::Float, TypeKind::Float, "-", TypeKind::Float},
        {TypeKind::Float, TypeKind::Float, "*", TypeKind::Float},
        {TypeKind::Float, TypeKind::Float, "/", TypeKind::Float},
        {TypeKind::Int, TypeKind::Float, "+", TypeKind::Float},
        {TypeKind::Int, TypeKind::Float, "-", TypeKind::Float},
        {TypeKind::Int, TypeKind::Float, "*", TypeKind::Float},
        {TypeKind::Int, TypeKind::Float, "/", TypeKind::Float},
        {TypeKind::Float, TypeKind::Int, "+", TypeKind::Float},
        {TypeKind::Float, TypeKind::Int, "-", TypeKind::Float},
        {TypeKind::Float, TypeKind::Int, "*", TypeKind::Float},
        {TypeKind::Float, TypeKind::Int, "/", TypeKind::Float},
        {TypeKind::String, TypeKind::String, "+", TypeKind::String},
        {TypeKind::String, TypeKind::Char, "+", TypeKind::String},
        {TypeKind::Char, TypeKind::String, "+", TypeKind::String},
        {TypeKind::String, TypeKind::Int, "*", TypeKind::String},
        {TypeKind::Int, TypeKind::String, "*", TypeKind::String},
        {TypeKind::Int, TypeKind::Int, "<<", TypeKind::Int},
        {TypeKind::Int, TypeKind::Int, ">>", TypeKind::Int}};

    for (auto const& rule : type_rules) {
      if (rule.l == lhs_type.kind && rule.r == rhs_type.kind && rule.op == ex->token.text) {
        return TypeInfo(rule.res);
      }
    }

    throw err::use_of_invalid_operator(ex->token, lhs_type.to_string(), rhs_type.to_string());
  }

  //
  // eval_typename
  //
  ExprTypeResult Sema::eval_typename(NdSymbol* node) {
    static constexpr std::pair<char const*, TypeKind> name_kind_pairs[]{
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

} // namespace superman::sema