#ifdef _FIRE_DEBUG_
#include <iostream>
#endif

#include "BuiltinFunc.hpp"
#include "Error.hpp"
#include "Node.hpp"
#include "Sema.hpp"
#include "Utils.hpp"

#define PRINT_LOCATION(TOK) (err::e(TOK, "node").print())

namespace fire {
TypeChecker::ArgumentsCompareResult TypeChecker::compare_arguments(
    NdCallFunc* cf, NdFunction* fn, BuiltinFunc const* builtin, bool is_var_arg,
    bool is_method_call, TypeInfo& self_ty, std::vector<TypeInfo> const& defs,
    std::vector<TypeInfo>& actual) {
  (void)cf;
  (void)fn;
  (void)builtin;
  (void)is_var_arg;
  (void)is_method_call;
  (void)self_ty;
  // defs   = 定義側
  // actual = 呼び出し側
  ArgumentsCompareResult result{};
  if (defs.size() < actual.size()) {
    if (!is_var_arg) result.flags |= ArgumentsCompareResult::TooMany;
  } else if (defs.size() > actual.size()) {
    result.flags |= ArgumentsCompareResult::TooFew;
  }
  for (size_t i = 0; i < defs.size(); i++) {
    if (defs[i].kind == TypeKind::Any) {
      continue;
    }
    if (!defs[i].equals(actual[i])) {
      result.flags |= ArgumentsCompareResult::TypeMismatch;
      result.mismatched_index = i;
      break;
    }
  }
  return result;
}

TypeInfo TypeChecker::case_call_func(NdCallFunc* cf, NdVisitorContext ctx) {
  // get argument types
  std::vector<TypeInfo> arg_types;
  for (auto& arg : cf->args) {
    ctx.as_arg_of_callfunc = true;
    arg_types.push_back(eval_expr_ty(arg, ctx));
  }
  ctx.as_arg_of_callfunc = false;
  TypeInfo self_ty;
  bool is_method_call = false;
  (void)is_method_call;
  size_t argc_give = arg_types.size();
  NdEnumeratorDef* nd_en_def = nullptr;
  ctx.enumerator_node_out = &nd_en_def;
  ctx.as_callee_of_callfunc = true;
  ctx.parent_cf_nd = cf;
  if (cf->is_method_call) {
    self_ty = eval_expr_ty(cf->inst_expr, ctx);
    ctx.self_ty_ptr = &self_ty;
    std::string const method_name{cf->callee->token.text};
    //
    // call method of class
    if (self_ty.is(TypeKind::Class)) {
      NdClass* nd_class = self_ty.class_node;
      for (NdFunction* method : nd_class->methods) {
        if (method->name.text == method_name) {
          std::vector<TypeInfo> arg_defs;
          for (auto arg : method->args) {
            arg_defs.push_back(eval_typename_ty(arg->type, ctx));
          }
          auto cmp = compare_arguments(cf, method, nullptr, method->is_var_args,
                                       true, self_ty, arg_defs, arg_types);
          if (cmp.flags & ArgumentsCompareResult::TypeMismatch) {
            throw err::mismatched_types(
                cf->args[cmp.mismatched_index]->token,
                arg_defs[cmp.mismatched_index].to_string(),
                arg_types[cmp.mismatched_index].to_string());
          }
          if (cmp.flags & ArgumentsCompareResult::TooMany) {
            throw err::too_many_arguments(
                cf->args[cmp.mismatched_index]->token);
          }
          if (cmp.flags & ArgumentsCompareResult::TooFew) {
            throw err::too_few_arguments(cf->args[cmp.mismatched_index]->token);
          }
          if (method->result_type)
            cf->ty = this->eval_typename_ty(method->result_type, ctx);
          cf->ty_evaluated = true;
          return cf->ty;
        }
      }
    }
    for (BuiltinFunc const* method : builtin_method_table) {
      if (method->name == method_name &&
          method->self_type.kind == self_ty.kind) {
        auto cmp =
            compare_arguments(cf, nullptr, method, method->is_var_args, true,
                              self_ty, method->arg_types, arg_types);
        if (cmp.flags & ArgumentsCompareResult::TypeMismatch) {
          throw err::mismatched_types(
              cf->args[cmp.mismatched_index]->token,
              method->arg_types[cmp.mismatched_index].to_string(),
              arg_types[cmp.mismatched_index].to_string());
        }
        if (cmp.flags & ArgumentsCompareResult::TooMany) {
          throw err::too_many_arguments(cf->args[cmp.mismatched_index]->token);
        }
        if (cmp.flags & ArgumentsCompareResult::TooFew) {
          throw err::too_few_arguments(cf->args[cmp.mismatched_index]->token);
        }
        cf->ty = method->returning_self ? self_ty : method->result_type;
        return cf->ty;
      }
    }
    throw err::e(cf->callee->token, "method '" + method_name +
                                        "' not found in '" +
                                        self_ty.to_string() + "'");
  }
  auto callee_ty =
      eval_expr_ty(cf->callee, ctx); // callee_ty = { result_type, [args...] }
  if (callee_ty.is(TypeKind::Class)) {
    NdClass* nd_class = callee_ty.class_node;
    if (argc_give > nd_class->fields.size()) {
      throw err::e(cf->args[argc_give]->token,
                   "too many arguments for constructor of '" +
                       callee_ty.to_string() + "'");
    } else if (argc_give < nd_class->fields.size()) {
      throw err::e(cf->args[argc_give]->token,
                   "too few arguments for constructor of '" +
                       callee_ty.to_string() + "'");
    }
    for (size_t i = 0; i < argc_give; i++) {
      // if not key-value-pair, fail.
      if (!cf->args[i]->is(NodeKind::KeyValuePair)) {
        throw err::e(cf->args[i]->token,
                     "expected field name in '" + callee_ty.to_string() + "'");
      }
      auto kvp = cf->args[i]->as<NdKeyValuePair>();
      // check name matching
      if (kvp->key->token.text != nd_class->fields[i]->name.text) {
        throw err::e(kvp->key->token, "field '" + kvp->key->token.text +
                                          "' is not found in '" +
                                          callee_ty.to_string() + "'");
      }
      // check type matching
      if (auto _expected = eval_typename_ty(nd_class->fields[i]->type, ctx);
          !arg_types[i].equals(_expected)) {
        throw err::mismatched_types(kvp->value->token, _expected.to_string(),
                                    arg_types[i].to_string());
      }
    }
    cf->ty = callee_ty;
    cf->ty_evaluated = true;
    return cf->ty;
  }
  if (callee_ty.is(TypeKind::Enum))
    return case_construct_enumerator(cf, nd_en_def, callee_ty, argc_give,
                                     arg_types, ctx);
  if (callee_ty.kind != TypeKind::Function) {
    todo; // callee must be function
  }
  size_t argc_take = callee_ty.parameters.size() - 1;
  if (!callee_ty.is_var_arg_functor && argc_take < argc_give) {
    throw err::too_many_arguments(cf->args[argc_take]->token);
  } else if (argc_take > argc_give) {
    throw err::too_few_arguments(cf->args[argc_take]->token);
  }
  for (size_t i = 0; i < argc_take; i++) {
    if (!arg_types[i].equals(callee_ty.parameters[i + 1])) {
      todo; // argument type mismatch
    }
  }
  cf->ty = callee_ty.parameters[0];
  return cf->ty;
}

TypeInfo TypeChecker::case_method_call(NdCallFunc* cf, NdVisitorContext ctx) {
  (void)cf;
  (void)ctx;
  todo;
}
TypeInfo TypeChecker::case_construct_enumerator(
    NdCallFunc* cf, NdEnumeratorDef* en_def, TypeInfo& callee_ty,
    size_t argc_give, std::vector<TypeInfo>& arg_types, NdVisitorContext ctx) {
  // no-variants
  if (en_def->type == NdEnumeratorDef::NoVariants) {
    err::emitters::enumerator_no_have_variants(cf->args[0]->token, en_def);
  }
  // one variant
  else if (en_def->type == NdEnumeratorDef::OneType) {
    if (argc_give == 0) {
      err::emitters::expected_one_variant_for_enumerator(cf->token, en_def);
    } else if (argc_give >= 2) {
      err::emitters::too_many_variants_for_enumerator(cf->args[0]->token,
                                                      en_def);
    } else if (argc_give == 1) {
      ctx = {};
      if (!arg_types[0].equals(eval_expr_ty(en_def->variant, ctx))) {
        throw err::mismatched_types(
            cf->args[0]->token,
            eval_typename_ty(en_def->variant, ctx).to_string(),
            arg_types[0].to_string());
      }
    }
  }
  // multiple variants or struct fields
  else if (en_def->type == NdEnumeratorDef::MultipleTypes ||
           en_def->type == NdEnumeratorDef::StructFields) {
    if (size_t count = en_def->multiple.size(); count > argc_give) {
      err::emitters::too_few_variants_for_enumerator(cf->args[0]->token,
                                                     en_def);
    } else if (count < argc_give) {
      err::emitters::too_many_variants_for_enumerator(cf->args[0]->token,
                                                      en_def);
    } else {
      for (size_t i = 0; i < count; i++) {
        auto expected_ty =
            en_def->multiple[i]->is(NodeKind::Symbol)
                ? eval_typename_ty(en_def->multiple[i]->as<NdSymbol>(), ctx)
                : eval_typename_ty(en_def->multiple[i]
                                       ->as<NdKeyValuePair>()
                                       ->value->as<NdSymbol>(),
                                   ctx);
        if (!arg_types[i].equals(expected_ty)) {
          throw err::mismatched_types(cf->args[i]->token,
                                      expected_ty.to_string(),
                                      arg_types[i].to_string());
        }
      }
    }
  }
  return cf->ty = callee_ty;
}

TypeInfo TypeChecker::eval_expr_ty(Node* node, NdVisitorContext ctx) {
  ctx.node = node;

  if (node->ty_evaluated) return node->ty;

#ifdef _FIRE_DEBUG_
    // err::e(
    //   node->token,
    //   format("### eval_expr_ty node=%p (kind=%d) (ctx = {%s})",
    //       node, static_cast<int>(node->kind),
    //       NdVisitorContext::ctx2s(ctx).c_str()),
    //   ET_Note).print();
#endif

  switch (node->kind) {
  case NodeKind::Value:
    node->ty = node->as<NdValue>()->obj->type;
    break;
  case NodeKind::Symbol: {
    auto sym = node->as<NdSymbol>();
    if (!sym->symbol_ptr) {
      throw err::use_of_undefined_symbol(sym->name);
      todo;
    }
    switch (sym->symbol_ptr->kind) {
    case SymbolKind::Enum:
    case SymbolKind::Class:
    case SymbolKind::BuiltinType:
    case SymbolKind::TemplateParam:
      sym->ty = eval_typename_ty(sym, ctx);
      break;
    case SymbolKind::Var:
      if (!sym->symbol_ptr->var_info->is_type_deducted) {
        debug(err::e(sym->token, "type not deduced").print());
        todo;
      }
      node->ty = sym->symbol_ptr->var_info->type;
      break;
    case SymbolKind::Func: {
      auto fn_scope = sym->symbol_ptr->scope->as<SCFunction>();
      auto fn_node = fn_scope->node->as<NdFunction>();
      node->ty = TypeInfo(TypeKind::Function);
      node->ty.parameters = {fn_node->result_type
                                 ? eval_typename_ty(fn_node->result_type, ctx)
                                 : TypeInfo()};
      for (auto a : fn_node->args)
        node->ty.parameters.emplace_back(eval_typename_ty(a->type, ctx));
      break;
    }
    case SymbolKind::Enumerator: {
      auto en = sym->symbol_ptr->node->as<NdEnumeratorDef>();
      assert(en->kind == NodeKind::EnumeratorDef);
      if (ctx.enumerator_node_out) {
        *ctx.enumerator_node_out = en;
      }
      node->ty = make_enum_type(en->parent_enum_node);
      break;
    }
    case SymbolKind::Namespace:
      todo;
    case SymbolKind::Module:
      todo; // !?!?
    case SymbolKind::BuiltinFunc:
      node->ty = sym->symbol_ptr->type;
      break;
    default:
      todo;
    }
    break;
  }
  case NodeKind::KeyValuePair: {
    auto kvp = node->as<NdKeyValuePair>();
    if (ctx.as_arg_of_callfunc) {
      return eval_expr_ty(kvp->value, ctx);
    }
    todo;
  }
  case NodeKind::Self: {
    if (!ctx.in_method) {
      throw err::semantics::cannot_use_self_here(node->token);
    }
    node->ty = make_class_type(ctx.cur_class->get_node());
    break;
  }
  case NodeKind::NullOpt: {
    node->ty = TypeInfo(TypeKind::NullOpt);
    break;
  }
  case NodeKind::Array: {
    auto arr = node->as<NdArray>();
    std::vector<TypeInfo> elems;
    for (auto& elem : arr->data) {
      elems.push_back(eval_expr_ty(elem, ctx));
    }
    if (elems.empty()) {
      if (!ctx.empty_array_element_type) {
        todo; // cannot deduce element type of empty array
      }
      node->ty = TypeInfo(TypeKind::Vector, {*ctx.empty_array_element_type},
                          false, false);
      break;
    }
    for (size_t i = 1; i < elems.size(); i++) {
      if (!elems[i].equals(ctx.can_use_empty_array
                               ? *ctx.empty_array_element_type
                               : elems[0])) {
        todo; // type mismatch
      }
    }
    node->ty = TypeInfo(TypeKind::Vector, {std::move(elems[0])}, false, false);
    break;
  }
  case NodeKind::Tuple: {
    auto tuple = node->as<NdTuple>();
    std::vector<TypeInfo> elems;
    for (auto& elem : tuple->elems) {
      elems.push_back(eval_expr_ty(elem, ctx));
    }
    node->ty = TypeInfo(TypeKind::Tuple, std::move(elems), false, false);
    break;
  }
  case NodeKind::CallFunc: {
    return case_call_func(node->as<NdCallFunc>(), ctx);
  }
  //
  // Subscript:
  case NodeKind::Subscript: {
    auto subs = node->as<NdExpr>();
    bool is_slice = subs->rhs->is(NodeKind::Slice);
    auto array_ty = eval_expr_ty(subs->lhs, ctx);
    if (is_slice) {
      auto slice = subs->rhs->as<NdExpr>();
      if (slice->lhs && !eval_expr_ty(slice->lhs, ctx).is(TypeKind::Int)) {
        todo; // index must be int
      }
      if (slice->rhs && !eval_expr_ty(slice->rhs, ctx).is(TypeKind::Int)) {
        todo; // end must be int
      }
      node->ty = array_ty;
      break;
    }
    auto index_ty = eval_expr_ty(subs->rhs, ctx);
    if (!index_ty.is(TypeKind::Int)) {
      todo; // index must be int
    }
    if (!array_ty.is(TypeKind::Vector)) {
      todo; // array must be vector
    }
    node->ty = array_ty.parameters[0];
    break;
  }
  case NodeKind::MemberAccess: {
    auto mm = node->as<NdExpr>();
    auto obj_ty = eval_expr_ty(mm->lhs, ctx);
    if (obj_ty.class_node == nullptr) {
      PRINT_LOCATION(mm->lhs->token);
      todo; // not instance.
    }
    NdClass* nd_class = obj_ty.class_node;
    std::string const field_name{mm->rhs->token.text};
    for (NdLet* field : nd_class->fields) {
      if (field->name.text == field_name) {
        node->ty = this->eval_typename_ty(field->type, ctx);
        node->ty_evaluated = true;
        return node->ty;
      }
    }
    throw err::e(mm->rhs->token, "field '" + field_name + "' not found in '" +
                                     obj_ty.to_string() + "'");
  }
  case NodeKind::GetTupleElement: {
    auto ge = node->as<NdGetTupleElement>();
    auto obj_ty = eval_expr_ty(ge->expr, ctx);
    if (!obj_ty.is(TypeKind::Tuple)) {
      throw err::mismatched_types(ge->token, "tuple", obj_ty.to_string());
    }
    if (ge->index < 0 || ge->index >= (int)obj_ty.parameters.size()) {
      err::emitters::tuple_getter_index_out_of_range(
          *ge->index_tok, ge->expr->token, obj_ty.to_string(), ge->index,
          (int)obj_ty.parameters.size());
    }
    node->ty = obj_ty.parameters[ge->index];
    break;
  }
  case NodeKind::Inclement:
  case NodeKind::Declement: {
    auto inc = node->as<NdInclementDeclement>();
    auto ty = eval_expr_ty(inc->expr, ctx);
    if (!ty.is(TypeKind::Int)) {
      throw err::mismatched_types(inc->token, "int", ty.to_string());
    }
    node->ty = ty;
    break;
  }
  case NodeKind::BitNot: {
    todo;
  }
  case NodeKind::Not: {
    auto x = node->as<NdOneExprWrap>();
    auto ty = eval_expr_ty(x->expr, ctx);
    if (!ty.is(TypeKind::Bool)) {
      throw err::mismatched_types(x->token, "bool", ty.to_string());
    }
    node->ty = ty;
    break;
  }
  case NodeKind::Ref: {
    todo;
  }
  case NodeKind::Deref: {
    auto deref = node->as<NdOneExprWrap>();
    TypeInfo obj_ty = eval_expr_ty(deref->expr, ctx);
    if (obj_ty.is(TypeKind::Option)) {
      node->ty = obj_ty.parameters[0];
    } else {
      throw err::mismatched_types(deref->token, "Option<T>",
                                  obj_ty.to_string());
    }
    break;
  }
  case NodeKind::Assign: {
    todo;
  }
  case NodeKind::AssignWithOp: {
    todo;
  }
  default: {
    assert(node->is_expr());
    auto ex = node->as<NdExpr>();
    auto lhs_ty = eval_expr_ty(ex->lhs, ctx);
    auto rhs_ty = eval_expr_ty(ex->rhs, ctx);
    // todo: check operators ...
    node->ty = lhs_ty;
    break;
  }
  }
  node->ty_evaluated = true;
  return node->ty;
}
TypeInfo TypeChecker::eval_typename_ty(NdSymbol* node, NdVisitorContext ctx) {
  ctx.node = node;

  switch (node->kind) {
  case NodeKind::Symbol: {
    auto sym = node->as<NdSymbol>();
    if (!sym->symbol_ptr) {
      debug(err::e(node->token, "sym->symbol_ptr is null").print());
      todo;
    }
    switch (sym->symbol_ptr->kind) {
    case SymbolKind::Enum: {
      return make_enum_type(sym->symbol_ptr->node->as<NdEnum>());
    }
    case SymbolKind::Class:
      return make_class_type(sym->symbol_ptr->node->as<NdClass>());
    case SymbolKind::BuiltinType: {
      TypeInfo ty = sym->symbol_ptr->type;
      for (auto p : sym->te_args) {
        ty.parameters.push_back(eval_expr_ty(p, ctx));
      }
      switch (ty.kind) {
      case TypeKind::Vector:
        if (ty.parameters.size() != 1) {
          todo; // vector must have one parameter
        }
        break;
      case TypeKind::Tuple:
        if (ty.parameters.size() == 0) {
          todo; // tuple must have parameters
        }
        break;
      case TypeKind::Dict:
        if (ty.parameters.size() != 2) {
          todo; // dict must have two parameters
        }
        break;
      case TypeKind::Option:
        if (ty.parameters.size() != 1) {
          todo; // option must have one parameter
        }
        break;
      }
      node->ty = ty;
      break;
    }
    case SymbolKind::TemplateParam:
      todo;
    default:
      err::emitters::expected_type_name_here(node->token);
    }
    break;
  }
  default:
    todo;
  }
  node->ty_evaluated = true;
  return node->ty;
}
TypeInfo TypeChecker::make_class_type(NdClass* node) {
  auto type = TypeInfo(TypeKind::Class);
  type.class_node = node;
  return type;
}
TypeInfo TypeChecker::make_enum_type(NdEnum* node) {
  auto type = TypeInfo(TypeKind::Enum);
  type.enum_node = node;
  debug(assert(node));
  return type;
}
void TypeChecker::check_expr(Node* node, NdVisitorContext ctx) {
  assert(node->is_expr_full());
  eval_expr_ty(node, ctx);
}
void TypeChecker::check_stmt(Node* node, NdVisitorContext ctx) {
  switch (node->kind) {
  case NodeKind::Scope: {
    check_scope(node->as<NdScope>(), ctx);
    break;
  }
  case NodeKind::Let: {
    auto let = node->as<NdLet>();
    if (!let->symbol_ptr) {
      PRINT_LOCATION(let->token);
    }
    assert(let->symbol_ptr);
    if (let->type) {
      let->symbol_ptr->var_info->type = eval_typename_ty(let->type, ctx);
      let->symbol_ptr->var_info->is_type_deducted = true;
    }
    if (let->init) {
      auto init_ty = eval_expr_ty(let->init, ctx);
      if (let->type) {
        if (!let->type->ty.equals(init_ty)) {
          throw err::mismatched_types(
              let->init->token, let->type->ty.to_string(), init_ty.to_string());
        }
      } else {
        let->symbol_ptr->var_info->type = init_ty;
        let->symbol_ptr->var_info->is_type_deducted = true;
      }
    }
    break;
  }
  case NodeKind::If: {
    auto if_ = node->as<NdIf>();
    auto cs = ctx.cur_scope;
    auto ifscope = if_->scope_ptr->as<SCIf>();
    ctx.cur_scope = ifscope;
    if (if_->vardef) check_stmt(if_->vardef, ctx);
    if (if_->cond) check_expr(if_->cond, ctx);
    check_scope(if_->thencode, ctx);
    if (if_->elsecode) {
      check_stmt(if_->elsecode, ctx);
    }
    ctx.cur_scope = cs;
    break;
  }
  case NodeKind::Match: {
    todo;
  }
  case NodeKind::For: {
    auto for_ = node->as<NdFor>();
    auto cs = ctx.cur_scope;
    auto forscope = for_->scope_ptr->as<SCFor>();
    ctx.loop_depth++;
    ctx.cur_scope = forscope;
    auto content_ty = eval_expr_ty(for_->iterable, ctx);
    switch (content_ty.kind) {
    case TypeKind::Vector:
    case TypeKind::List:
      forscope->iter_name->var_info->type = content_ty.parameters[0];
      forscope->iter_name->var_info->is_type_deducted = true;
      break;
    case TypeKind::String:
      forscope->iter_name->var_info->type = TypeInfo(TypeKind::Char);
      forscope->iter_name->var_info->is_type_deducted = true;
      break;
    default:
      err::emitters::expected_iterable_type(for_->iterable->token,
                                            content_ty.to_string());
    }
    check_scope(for_->body, ctx);
    ctx.cur_scope = cs;
    ctx.loop_depth--;
    break;
  }
  case NodeKind::While: {
    NdWhile* nd_while = node->as<NdWhile>();
    auto cs = ctx.cur_scope;
    ctx.loop_depth++;
    ctx.cur_scope = nd_while->scope_ptr;
    if (nd_while->vardef) check_stmt(nd_while->vardef, ctx);
    if (nd_while->cond) check_expr(nd_while->cond, ctx);
    check_scope(nd_while->body, ctx);
    ctx.cur_scope = cs;
    ctx.loop_depth--;
    break;
  }
  case NodeKind::Loop: {
    todo;
  }
  case NodeKind::Try: {
    NdTry* nd_try = node->as<NdTry>();
    check_scope(nd_try->body, ctx);
    for (NdCatch* nd_catch : nd_try->catches) {
      TypeInfo holder_error_ty = eval_typename_ty(nd_catch->error_type, ctx);
      (void)holder_error_ty;
      check_scope(nd_catch->body, ctx);
    }
    if (nd_try->finally_block) {
      check_scope(nd_try->finally_block, ctx);
    }
    todo;
  }
  case NodeKind::Break: {
    todo;
  }
  case NodeKind::Continue: {
    todo;
  }
  case NodeKind::Return: {
    auto nd_ret = node->as<NdReturn>();
    assert(ctx.expected_return_type);
    nd_ret->ty = this->eval_expr_ty(nd_ret->expr, ctx);
    nd_ret->ty_evaluated = true;
    break;
  }
  default:
    // alertexpr(static_cast<int>(node->kind));
    assert(node->is_expr_full());
    check_expr(node, ctx);
    break;
  }
}
void TypeChecker::check_scope(NdScope* node, NdVisitorContext ctx) {
  ctx.cur_scope = node->scope_ptr;
  for (auto& item : node->items) {
    switch (item->kind) {
    case NodeKind::Scope:
      check_scope(item->as<NdScope>(), ctx);
      break;
    default:
      check_stmt(item, ctx);
      break;
    }
  }
}
void TypeChecker::check_function(NdFunction* node, NdVisitorContext ctx) {
  auto fn_scope = node->scope_ptr->as<SCFunction>();
  (void)fn_scope;
  for (auto& arg : node->args) {
    arg->type->ty = eval_typename_ty(arg->type, ctx);
    arg->type->ty_evaluated = true;
    arg->var_info_ptr->type = arg->type->ty;
    arg->var_info_ptr->is_type_deducted = true;
  }
  if (node->result_type) {
    node->result_type->ty = eval_typename_ty(node->result_type, ctx);
    node->result_type->ty_evaluated = true;
    ctx.expected_return_type = &node->result_type->ty;
  }
  check_scope(node->body, ctx);
}
void TypeChecker::check_class(NdClass* node, NdVisitorContext ctx) {
  ctx.cur_class = node->scope_ptr->as<SCClass>();
  for (auto field : node->fields) {
    check_stmt(field, ctx);
  }
  for (auto method : node->methods) {
    ctx.in_method = method->take_self;
    check_function(method, ctx);
  }
  ctx.in_method = false;
}
void TypeChecker::check_enum(NdEnum* node, NdVisitorContext ctx) {
  (void)node;
  (void)ctx;
}
void TypeChecker::check_namespace(NdNamespace* node, NdVisitorContext ctx) {
  ctx.cur_scope = node->scope_ptr->as<SCNamespace>();
  for (auto& item : node->items) {
    switch (item->kind) {
    case NodeKind::Let:
      check_stmt(item, ctx);
      break;
    case NodeKind::Function:
      check_function(item->as<NdFunction>(), ctx);
      break;
    case NodeKind::Class:
      check_class(item->as<NdClass>(), ctx);
      break;
    case NodeKind::Enum:
      check_enum(item->as<NdEnum>(), ctx);
      break;
    case NodeKind::Namespace:
      check_namespace(item->as<NdNamespace>(), ctx);
      break;
    default:
      todo;
    }
  }
}
void TypeChecker::check_module(NdModule* node, NdVisitorContext ctx) {
  ctx.cur_scope = node->scope_ptr->as<SCModule>();
  for (auto& item : node->items) {
    switch (item->kind) {
    case NodeKind::Let:
      check_stmt(item, ctx);
      break;
    case NodeKind::Function:
      check_function(item->as<NdFunction>(), ctx);
      break;
    case NodeKind::Class:
      check_class(item->as<NdClass>(), ctx);
      break;
    case NodeKind::Enum:
      check_enum(item->as<NdEnum>(), ctx);
      break;
    case NodeKind::Namespace:
      check_namespace(item->as<NdNamespace>(), ctx);
      break;
    default:
      todo;
    }
  }
}
} // namespace fire
