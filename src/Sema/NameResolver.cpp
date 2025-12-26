#include "Sema/Sema.hpp"
#include "Error.hpp"
#include "Utils.hpp"

namespace fire {

  void NameResolver::on_typename(Node* node, NdVisitorContext ctx) {
    switch (node->kind) {
      case NodeKind::Symbol: {
        auto sym = node->as<NdSymbol>();
        if (sym->dec)
          on_expr(sym->dec->expr, ctx);
        on_expr(sym, ctx);
        break;
      }
    }
  }

  void NameResolver::on_expr(Node* node, NdVisitorContext ctx) {
    switch (node->kind) {
      case NodeKind::Symbol: {
        auto sym = node->as<NdSymbol>();
        auto result = S.find_symbol(sym, ctx);

        if (result.hits.size() == 1) {
          sym->symbol_ptr = result.hits[0];
        }

        for (auto template_param : sym->te_args)
          on_typename(template_param, ctx);

        break;
      }

      case NodeKind::KeyValuePair: {
        auto kvp = node->as<NdKeyValuePair>();
        on_expr(kvp->key, ctx);
        on_expr(kvp->value, ctx);
        break;
      }

      case NodeKind::Self: {
        break;
      }

      case NodeKind::Array: {
        auto arr = node->as<NdArray>();
        for (auto& item : arr->data)
          on_expr(item, ctx);
        break;
      }

      case NodeKind::Tuple: {
        auto tuple = node->as<NdTuple>();
        for (auto& item : tuple->elems)
          on_expr(item, ctx);
        break;
      }

      case NodeKind::Slice: {
        auto slice = node->as<NdExpr>();
        on_expr(slice->lhs, ctx);
        on_expr(slice->rhs, ctx);
        break;
      }

      case NodeKind::MemberAccess: {
        auto mem = node->as<NdExpr>();

        on_expr(mem->lhs, ctx);

        on_expr(mem->rhs, ctx);

        break;
      }

      case NodeKind::CallFunc: {
        auto call = node->as<NdCallFunc>();

        on_expr(call->callee, ctx);

        for (auto& arg : call->args)
          on_expr(arg, ctx);

        break;
      }

      case NodeKind::GetTupleElement: {
        auto get = node->as<NdGetTupleElement>();
        on_expr(get->expr, ctx);
        break;
      }

      case NodeKind::Inclement: {
        auto inc = node->as<NdInclement>();
        on_expr(inc->expr, ctx);
        break;
      }

      case NodeKind::Declement: {
        auto dec = node->as<NdDeclement>();
        on_expr(dec->expr, ctx);
        break;
      }

      case NodeKind::BitNot: {
        auto bitnot = node->as<NdBitNot>();
        on_expr(bitnot->expr, ctx);
        break;
      }

      case NodeKind::Not: {
        auto x = node->as<NdNot>();
        on_expr(x->expr, ctx);
        break;
      }

      case NodeKind::Ref: {
        auto ref = node->as<NdRef>();
        on_expr(ref->expr, ctx);
        break;
      }

      case NodeKind::Deref: {
        auto deref = node->as<NdDeref>();
        on_expr(deref->expr, ctx);
        break;
      }

      case NodeKind::New: {
        auto x = node->as<NdNew>();
        on_expr(x->type, ctx);
        for (auto& arg : x->args)
          on_expr(arg, ctx);
        break;
      }

      case NodeKind::Mul:
      case NodeKind::Div:
      case NodeKind::Mod:
      case NodeKind::Add:
      case NodeKind::Sub:
      case NodeKind::LShift:
      case NodeKind::RShift:
      case NodeKind::BitAnd:
      case NodeKind::BitXor:
      case NodeKind::BitOr:
      case NodeKind::LogAnd:
      case NodeKind::LogOr:
      case NodeKind::Equal:
      case NodeKind::Bigger:
      case NodeKind::BiggerOrEqual:
      case NodeKind::Assign: {
        auto x = node->as<NdExpr>();
        on_expr(x->lhs, ctx);
        on_expr(x->rhs, ctx);
        break;
      }

      case NodeKind::AssignWithOp: {
        auto x = node->as<NdAssignWithOp>();
        on_expr(x->lhs, ctx);
        on_expr(x->rhs, ctx);
        break;
      }
    }
  }

  void NameResolver::on_stmt(Node* node, NdVisitorContext ctx) {
    switch (node->kind) {
      case NodeKind::Scope: {
        on_scope(node, ctx);
        break;
      }

      case NodeKind::Let: {
        auto x = node->as<NdLet>();

        if (x->type)
          on_typename(x->type, ctx);

        if (x->init)
          on_expr(x->init, ctx);
        break;
      }

      case NodeKind::If: {
        auto x = node->as<NdIf>();
        if (x->vardef)
          on_stmt(x->vardef, ctx);
        if (x->cond)
          on_expr(x->cond, ctx);
        on_stmt(x->thencode, ctx);
        if (x->elsecode)
          on_stmt(x->elsecode, ctx);
        break;
      }

      case NodeKind::For: {
        auto x = node->as<NdFor>();
        ctx.loop_depth++;
        auto cs = ctx.cur_scope;
        ctx.cur_scope = x->scope_ptr;
        on_expr(x->iterable, ctx);
        on_stmt(x->body, ctx);
        ctx.loop_depth--;
        ctx.cur_scope = cs;
        break;
      }

      case NodeKind::While: {
        auto x = node->as<NdWhile>();
        ctx.loop_depth++;
        if (x->vardef)
          on_stmt(x->vardef, ctx);
        if (x->cond)
          on_expr(x->cond, ctx);
        on_stmt(x->body, ctx);
        break;
      }

      case NodeKind::Break: {
        if (ctx.loop_depth == 0)
          throw err::semantics::cannot_use_break_here(node->token);
        break;
      }

      case NodeKind::Continue: {
        if (ctx.loop_depth == 0)
          throw err::semantics::cannot_use_continue_here(node->token);
        break;
      }

      case NodeKind::Return: {
        auto x = node->as<NdReturn>();
        if (x->expr)
          on_expr(x->expr, ctx);
        break;
      }

      case NodeKind::Try: {
        auto x = node->as<NdTry>();
        on_scope(x->body, ctx);
        for (auto& catch_ : x->catches) {
          on_typename(catch_->error_type, ctx);
          on_scope(catch_->body, ctx);
        }
        if (x->finally_block)
          on_scope(x->finally_block, ctx);
        break;
      }

      default:
        assert(node->is_expr_full());
        on_expr(node, ctx);
        break;
    }
  }

  void NameResolver::on_scope(Node* node, NdVisitorContext ctx) {
    auto scope = node->as<NdScope>();

    ctx.cur_scope = scope->scope_ptr;

    for (auto& item : scope->items) {
      on_stmt(item, ctx);
    }
  }

  void NameResolver::on_function(Node* node, NdVisitorContext ctx) {
    auto func = node->as<NdFunction>();

    ctx.cur_func = func->scope_ptr->as<SCFunction>();
    ctx.cur_scope = ctx.cur_func;

    for (auto arg : func->args) {
      on_typename(arg.type, ctx);
    }

    if (func->result_type)
      on_typename(func->result_type, ctx);

    on_scope(func->body, ctx);
  }

  void NameResolver::on_class(Node* node, NdVisitorContext ctx) {
    auto C = node->as<NdClass>();

    ctx.cur_class = C->scope_ptr->as<SCClass>();
    ctx.cur_scope = ctx.cur_class;

    for (auto& field : C->fields) {
      on_stmt(field, ctx);
    }
    for (auto& method : C->methods) {
      on_function(method, ctx);
    }
  }

  void NameResolver::on_enum(Node* node, NdVisitorContext ctx) {
    auto E = node->as<NdEnum>();

    ctx.cur_scope = E->scope_ptr->as<SCEnum>();

    for (auto& en : E->enumerators) {
      switch(en->type){
        case NdEnumeratorDef::MultipleTypes:
          for(auto&i:en->multiple)on_typename(i,ctx);
          break;
        case NdEnumeratorDef::StructFields:
          for(auto&i:en->multiple)on_typename(i->as<NdKeyValuePair>()->value->as<NdSymbol>(),ctx);
          break;
        case NdEnumeratorDef::OneType:
          on_typename(en->variant,ctx);
      }
    }
  }

  void NameResolver::on_namespace(Node* node, NdVisitorContext ctx) {
    auto NS = node->as<NdNamespace>();

    ctx.cur_scope = NS->scope_ptr->as<SCNamespace>();

    for (auto& item : NS->items) {
      switch (item->kind) {
        case NodeKind::Let:
          on_stmt(item, ctx);
          break;
        case NodeKind::Function:
          on_function(item, ctx);
          break;
        case NodeKind::Class:
          on_class(item, ctx);
          break;
        case NodeKind::Enum:
          on_enum(item, ctx);
          break;
        case NodeKind::Namespace:
          on_namespace(item, ctx);
          break;
      }
    }
  }

  void NameResolver::on_module(Node* node, NdVisitorContext ctx) {
    auto M = node->as<NdModule>();

    auto modscope = M->scope_ptr->as<SCModule>();

    ctx.cur_scope = modscope;

    for (auto& item : M->items) {
      switch (item->kind) {
        case NodeKind::Let:
          on_stmt(item, ctx);
          break;
        case NodeKind::Function:
          on_function(item, ctx);
          break;
        case NodeKind::Class:
          on_class(item, ctx);
          break;
        case NodeKind::Enum:
          on_enum(item, ctx);
          break;
        case NodeKind::Namespace:
          on_namespace(item, ctx);
          break;
      }
    }
  }

} // namespace fire