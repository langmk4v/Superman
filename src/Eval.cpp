#include "Eval.hpp"

namespace superman {

  Evaluator::CallStack& Evaluator::push_stack() {
    return *call_stack.emplace_back(new CallStack);
  }

  void Evaluator::pop_stack() {
    delete call_stack.back();
    call_stack.pop_back();
  }

  Object* Evaluator::eval_node(Node* node) {
    switch (node->kind) {

    case NodeKind::Module:
    case NodeKind::Function:
    case NodeKind::Class:
    case NodeKind::Enum:
      break;

    case NodeKind::Value:
      return node->as<NdValue>()->obj;
    }

    return Object::none;
  }

} // namespace superman