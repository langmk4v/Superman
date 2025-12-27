#include "EvalNode.hpp"
namespace fire {
  NodeEvaluator::NodeEvaluator(NodeEvaluatorConfig config) : config(config) {}
  Object*&NodeEvaluator::eval_lvalue(Node*node){
    todo;
  }
  Object* NodeEvaluator::eval_expr(Node* node) {
    switch (node->kind) {
      case NodeKind::Value: return node->as<NdValue>()->obj;
      case NodeKind::Symbol: {
        auto sym = node->as<NdSymbol>();
        switch (sym->kind) {}
        break;
      }
      case NodeKind::KeyValuePair: {
        break;
      }
      case NodeKind::Self: {
        break;
      }
      case NodeKind::Array: {
        break;
      }
      case NodeKind::Tuple: {
        break;
      }
      case NodeKind::Slice: {
        break;
      }
      case NodeKind::Subscript: {
        break;
      }
      case NodeKind::MemberAccess: {
        break;
      }
      case NodeKind::CallFunc: {
        break;
      }
      case NodeKind::GetTupleElement: {
        break;
      }
      case NodeKind::Inclement: {
        break;
      }
      case NodeKind::Declement: {
        break;
      }
      case NodeKind::BitNot: {
        break;
      }
      case NodeKind::Not: {
        break;
      }
      case NodeKind::Ref: {
        break;
      }
      case NodeKind::Deref: {
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
          add_int_int:
            return new ObjInt(lhs->as_int()->val + rhs->as_int()->val);
          }
          if (rk == TypeKind::Float) {
          add_int_float:
            return new ObjFloat(static_cast<double>(lhs->as_int()->val) + rhs->as_float()->val);
          }
        }
        if (lk == TypeKind::Float) {}
        todo;
      }
      case NodeKind::Sub: {
      }
    }
    stop;
  }
  Object* NodeEvaluator::eval_stmt(Node* node) { return nullptr; }
  Object* NodeEvaluator::eval_scope(NdScope* node) { return nullptr; }
  std::shared_ptr<NodeEvaluator::CallStack> NodeEvaluator::push_call_stack() {
    auto call_stack = std::make_shared<CallStack>();
    this->call_stack.push_back(call_stack);
    return call_stack;
  }
  void NodeEvaluator::pop_call_stack() { this->call_stack.pop_back(); }
  NodeEvaluator::CallStack& NodeEvaluator::current_stack() { return *this->call_stack.back(); }
} // namespace fire