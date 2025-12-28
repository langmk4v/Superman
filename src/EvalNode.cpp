#ifdef _FIRE_DEBUG_
#include <iostream>
#endif

#include "Utils.hpp"
#include "Error.hpp"
#include "EvalNode.hpp"

namespace fire {

NodeEvaluator::NodeEvaluator(NodeEvaluatorConfig config) : config(config) {}

Object*& NodeEvaluator::eval_lvalue(Node* node) {
  (void)node;
  todo;
}

Object* NodeEvaluator::eval_expr(Node* node) {

#if _FIRE_DEBUG_
  // err::e(node->token, "eval_expr", ET_Note).print();
#endif

  switch (node->kind) {
  case NodeKind::Value:
    return node->as<NdValue>()->obj;
  case NodeKind::Symbol: {
    auto sym = node->as<NdSymbol>();
    switch (sym->kind) {}
    break;
  }
  case NodeKind::KeyValuePair: {
    todo;
    break;
  }
  case NodeKind::Self: {
    todo;
    break;
  }
  case NodeKind::Array: {
    todo;
    break;
  }
  case NodeKind::Tuple: {
    todo;
    break;
  }
  case NodeKind::Slice: {
    todo;
    break;
  }
  case NodeKind::Subscript: {
    todo;
    break;
  }
  case NodeKind::MemberAccess: {
    todo;
    break;
  }
  case NodeKind::CallFunc: {
    todo;
    break;
  }
  case NodeKind::GetTupleElement: {
    todo;
    break;
  }
  case NodeKind::Inclement: {
    todo;
    break;
  }
  case NodeKind::Declement: {
    todo;
    break;
  }
  case NodeKind::BitNot: {
    todo;
    break;
  }
  case NodeKind::Not: {
    todo;
    break;
  }
  case NodeKind::Ref: {
    todo;
    break;
  }
  case NodeKind::Deref: {
    todo;
    break;
  }
  case NodeKind::Assign: {
    auto ex = node->as<NdExpr>();
    return this->eval_lvalue(ex->lhs) = this->eval_expr(ex->rhs);
  }
  case NodeKind::AssignWithOp: {
    todo;
  }
  }
  assert(node->is_expr());
  NdExpr* expr = node->as<NdExpr>();
  Object *lhs = this->eval_expr(expr->lhs), *rhs = this->eval_expr(expr->rhs);
  TypeKind lk = lhs->type.kind, rk = rhs->type.kind;
  switch (expr->kind) {
  case NodeKind::Add: {
    if (lk == TypeKind::Int) {
      if (rk == TypeKind::Int) {
        return new ObjInt(lhs->as_int()->val + rhs->as_int()->val);
      }
      if (rk == TypeKind::Float) {
      add_int_float:
        return new ObjFloat(static_cast<double>(lhs->as_int()->val) +
                            rhs->as_float()->val);
      }
    }
    if (lk == TypeKind::Float) {
      if (rk == TypeKind::Int) {
        std::swap(lhs, rhs);
        goto add_int_float;
      }
    }
    todo;
  }
  case NodeKind::Sub: {
  }
  }
  stop;
}

void NodeEvaluator::eval_stmt(Node* node) {
  switch (node->kind) {
  case NodeKind::Scope:
    eval_scope(node->as<NdScope>());
    break;

  case NodeKind::Let: {
    todo;
    break;
  }

  default:
    alertexpr(static_cast<int>(node->kind));
    assert(node->is_expr_full());
    this->eval_expr(node);
    break;
  }
}

void NodeEvaluator::eval_scope(NdScope* node) {
  auto scope_stack = this->push_scope_stack();

  for (auto n : node->items)
    this->eval_stmt(n);

  this->pop_call_stack();
}

std::shared_ptr<NodeEvaluator::CallStack> NodeEvaluator::push_call_stack() {
  auto call_stack = std::make_shared<CallStack>();
  this->call_stack.push_back(call_stack);
  return call_stack;
}

void NodeEvaluator::pop_call_stack() { this->call_stack.pop_back(); }

NodeEvaluator::CallStack& NodeEvaluator::current_stack() {
  return *this->call_stack.back();
}

std::shared_ptr<NodeEvaluator::ScopeStack> NodeEvaluator::push_scope_stack() {
  return this->current_stack().scope_stack.emplace_back(
      std::make_shared<ScopeStack>());
}

void NodeEvaluator::pop_scope_stack() {
  this->current_stack().scope_stack.pop_back();
}

NodeEvaluator::ScopeStack& NodeEvaluator::current_scope_stack() {
  return *this->current_stack().scope_stack.back();
}

} // namespace fire