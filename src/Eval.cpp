#include "Eval.hpp"
#include "BuiltinFunc.hpp"

namespace superman {

  Evaluator::CallStack::CallStack(int vn) { variables.resize(vn); }

  Evaluator::CallStack::~CallStack() {}

  Evaluator::CallStack& Evaluator::push_stack(int var_count) {
    return *call_stack.emplace_back(new CallStack(var_count));
  }

  void Evaluator::pop_stack() {
    delete call_stack.back();
    call_stack.pop_back();
  }

  void Evaluator::eval_stmt(Node* node) {
    switch (node->kind) {

    case NodeKind::Module:
    case NodeKind::Function:
    case NodeKind::Class:
    case NodeKind::Enum:
      break;

    case NodeKind::Scope: {
      auto scope = node->as<NdScope>();

      for (auto s : scope->items) {
        eval_stmt(s);
      }

      break;
    }

    case NodeKind::Let: {
      auto let = node->as<NdLet>();

      if (let->init) cur_stack().variables[let->index] = eval_expr(let->init);

      break;
    }

    default:
      eval_expr(node);
      break;
    }
  }

  Object* Evaluator::eval_expr(Node* node) {

    assert(node != nullptr);

    switch (node->kind) {
    case NodeKind::Value:
      return node->as<NdValue>()->obj;

    case NodeKind::Symbol: {
      auto sym = node->as<NdSymbol>();

      switch (sym->type) {
      case NdSymbol::Var:
        if (sym->is_global_var) return globals[sym->var_offset];
        return cur_stack().variables[sym->var_offset];
      }

      todoimpl;
    }

    case NodeKind::CallFunc: {
      auto cf = node->as<NdCallFunc>();

      (void)cf;

      std::vector<Object*> args;

      for (auto&& a : cf->args)
        args.push_back(eval_expr(a));

      if (cf->blt_fn) {
        return cf->blt_fn->impl(args);
      }

      todoimpl;
    }

    default:
      todoimpl;
    }

    return Object::none;
  }

  void Evaluator::add_global_var(NdLet* let) {
    globals.push_back(let->init ? eval_expr(let->init) : nullptr);
  }

} // namespace superman