#include <algorithm>

#include "Utils/macro.h"

#include "VM/Interp/Object.hpp"
#include "Driver/Error.hpp"
#include "Sema/Sema.hpp"

namespace fire::sema {

  using namespace lexer;
  using namespace parser;

  using vm::interp::BuiltinFunc;

  //
  // eval_expr
  //
  ExprType Sema::eval_expr(Node* node) {
    switch (node->kind) {

    //
    // Value
    //
    case NodeKind::Value:
      return { node, node->as<NdValue>()->obj->type };

    case NodeKind::Self:{
      auto method = this->get_cur_func_scope();

      if(!method)
        throw err::semantics::cannot_use_self_here(node->token);
      else if( !method->is_method)
        throw err::semantics::cannot_use_self_in_not_class_method(node->token);
      
      return { node, this->make_class_type(method->parent->node->as<NdClass>()) };
    }

    //
    // Symbol
    //
    case NodeKind::Symbol: {
      auto sym = node->as<NdSymbol>();

      SymbolFindResult result = find_symbol(sym);

      if (result.empty()) {

        if(result.builtin_f) {
          sym->builtin_f = result.builtin_f;

          TypeInfo ti { TypeKind::Function, result.builtin_f->arg_types, false, false };

          ti.parameters.insert(ti.parameters.begin(), result.builtin_f->result_type);

          ExprType res { node, std::move(ti) };

          res.builtin_f = result.builtin_f;

          return res;
        }

        throw err::use_of_undefined_symbol(sym->name);
      }

      if (result.count() >= 2)
        throw err::ambiguous_symbol_name(sym->name);

      if (result.count()) {
        auto S = result.matches[0];

        sym->sym_target = S->node;

        switch (S->kind) {
        case SymbolKind::Var: {
          if (!S->var_info->is_argument() && !S->var_info->is_type_deducted) {
            todo;
          }

          debug(
            if (S->var_info->is_argument())
              assert(S->var_info->is_type_deducted);
          )

          (S->var_info->is_global ? sym->is_global_var : sym->is_local_var) = true;

          sym->var_offset = S->var_info->offset;

          return { node, S->var_info->type };
        }

        case SymbolKind::Func: {
          auto f = S->node->as<NdFunction>();

          assert(f->is(NodeKind::Function));

          auto ti = TypeInfo(TypeKind::Function,
                             {f->result_type ? eval_typename(f->result_type).type : TypeKind::None},
                             false, false);

          for (auto&& arg : f->args)
            ti.parameters.emplace_back(eval_typename(arg.type).type);

          auto res = ExprType(node, ti);

          res.func_nd = f;

          return res;
        }

        default:
          todo;
        }
      }

      todo;
    }

    //
    // CallFunc
    //
    case NodeKind::CallFunc: {
      auto cf = node->as<NdCallFunc>();

      ExprType callee = eval_expr(cf->callee);

      if (callee.fail()) {
        todo;
      }

      if (callee.type.kind != TypeKind::Function) {
        throw err::not_callable_type(cf->callee->token, callee.type.to_string());
      }

      bool is_blt = callee.builtin_f != nullptr;

      if (callee.func_nd)
        cf->func_nd = callee.func_nd;

      auto calls = (int)cf->args.size();
      auto takes = (int)callee.type.parameters.size() - 1;

      if (calls != takes) {
        bool is_var_arg = is_blt && callee.builtin_f->is_var_args;

        if (!is_var_arg) {
          // todo: get isvararg flag of user-def func
        }

        if (is_var_arg ? (calls < takes - 1) : (calls < takes))
          throw err::too_few_arguments(cf->callee->token);
        else if (!is_var_arg && calls > takes)
          throw err::too_many_arguments(cf->callee->token);
      }

      for (int i = 0; i < std::min(calls, takes); i++) {
        TypeInfo argtype = eval_expr(cf->args[i]).type;

        if (!argtype.equals(callee.type.parameters[i + 1]))
          throw err::mismatched_types(cf->args[i]->token, callee.type.parameters[i + 1].to_string(),
                                      argtype.to_string());
      }

      for (int i = takes; i < calls; i++) {
        eval_expr(cf->args[i]);
      }

      return { node, callee.type.parameters[0] };
    }

    case NodeKind::MemberAccess: {
      auto x = node->as<NdExpr>();

      assert(x->rhs->is(NodeKind::Symbol));

      ExprType inst = eval_expr(x->lhs);

      if (inst.type.kind != TypeKind::Class) {
        throw err::semantics::expected_class_type(x->lhs->token);
      }

      NdClass* class_node = inst.type.class_node;

      assert(class_node);

      NdSymbol* right = x->rhs->as<NdSymbol>();

      if(right->next){
        // TODO: <基底クラス名> "::" <メンバ名>
        todo;
      }

      for(NdLet*f:class_node->fields){
        if(f->name.text == right->name.text){
          return this->eval_typename(f->type);
        }
      }

      throw err::semantics::not_field_of_class(right->token,right->name.text,class_node->name.text);
    }

    case NodeKind::New: {
      auto new_nd = node->as<NdNew>();

      ExprType class_type = eval_typename(new_nd->type);

      if (!class_type.class_nd) {
        todo;
      }

      ExprType result = ExprType();

      result.type = TypeKind::Class;
      result.type.class_node = class_type.class_nd;

      result.node = node;

      // printd(result.type.to_string());

      return result;
    }

    case NodeKind::Assign: {
      auto as= node->as<NdExpr>();

      auto left = eval_expr(as->lhs).type;
      auto right = eval_expr(as->rhs).type;

      if(!left.equals(right)){
        throw err::semantics::not_same_type_assignment(as->token, left.to_string(), right.to_string());
      }

      return ExprType(node,left);
    }
    }

    if (!node->is_expr()) {
      todo;
    }

    NdExpr* ex = node->as<NdExpr>();

    ExprType left_e = eval_expr(ex->lhs);
    ExprType right_e = eval_expr(ex->rhs);

    TypeInfo& left = left_e.type;
    TypeInfo& right = right_e.type;

    switch (ex->kind) {
    case NodeKind::Add:
      if (left.is(TypeKind::String) && right.is(TypeKind::String))
        return { node, TypeInfo(TypeKind::String) };
      break;

    case NodeKind::Mul:
      if ((left.is(TypeKind::Int) && right.is(TypeKind::String)) ||
          (left.is(TypeKind::String) && right.is(TypeKind::Int)))
        return { node, TypeInfo(TypeKind::String) };
      break;

    case NodeKind::Mod:
    case NodeKind::LShift:
    case NodeKind::RShift:
      if (left.is(TypeKind::Float) || right.is(TypeKind::Float)) goto error_calc;
      break;

    case NodeKind::Sub:
    case NodeKind::Div:
      break;

    case NodeKind::Equal:
      if (!left.equals(right)) goto error_calc;
      break;
    }

    if (left.is_numeric() && right.is_numeric()) {
      return {
        node,
        TypeInfo(left.is(TypeKind::Float) || right.is(TypeKind::Float) ? TypeKind::Float : TypeKind::Int)
      };
    }

  error_calc:
    throw err::use_of_invalid_operator(ex->token, left.to_string(), right.to_string());
  }

  //
  // eval_typename
  //
  ExprType Sema::eval_typename(NdSymbol* node) {
    static constexpr std::pair<char const*, TypeKind> name_kind_pairs[]{
        {"none", TypeKind::None},         {"int", TypeKind::Int},     {"float", TypeKind::Float},
        {"bool", TypeKind::Bool},         {"char", TypeKind::Char},   {"string", TypeKind::String},
        {"vector", TypeKind::Vector},     {"tuple", TypeKind::Tuple}, {"dict", TypeKind::Dict},
        {"function", TypeKind::Function},
    };

    ExprType result = ExprType(node);

    if (node->dec) {
      try {
        return eval_expr(node->dec->expr);
      }
      catch (err::e e) {
        if (node->dec->expr->is(NodeKind::Symbol) && eval_typename(node->dec->expr->as<NdSymbol>()).is_succeed)
          throw err::semantics::expected_expr_but_found(node->dec->expr->token, "type-name");
        else
          throw e;
      }
    }

    // 基本型の名前から探す
    for (auto&& [s, k] : name_kind_pairs)
      if (node->name.text == s) {
        result.type.kind = k;

        // テンプレート引数が多すぎるまたは少なすぎる
        if (int C = TypeInfo::required_param_count(k), N = (int)node->te_args.size(); C != N)
          throw err::no_match_template_arguments(node->name, C, N);

        // 存在したらユーザー定義型の検索はスキップ
        result.is_succeed = true;
        return result;
      }

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

    auto sym = found[0];

    switch (sym->kind) {
      case SymbolKind::Class: {
        result.class_nd = sym->node->as<NdClass>();
        
        result.type = TypeKind::Class;
        result.type.class_node = result.class_nd;

        result.is_succeed = true;

        break;
      }

      default:
        throw err::semantics::this_is_not_typename(node->token);
    }

    return result;
  }

} // namespace superman::sema