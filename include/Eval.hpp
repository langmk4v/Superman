#pragma once

#include <unordered_map>

#include "macro.h"

#include "Node.hpp"
#include "Object.hpp"

namespace superman {
  class Evaluator {

    struct CallStack {
      std::vector<Object*> variables;

      CallStack(int vn);
      ~CallStack();
    };

    std::vector<Object*> globals;

    std::vector<CallStack*> call_stack;

  public:
    Evaluator() {}

    void eval_stmt(Node* node);

    Object* eval_expr(Node* node);

    CallStack& push_stack(int var_count);

    void pop_stack();

    void add_global_var(NdLet*);

    std::tuple<NdExpr*, Object*, Object*> get_expr_tu(Node* ndexpr) {
      auto x = ndexpr->as<NdExpr>();
      return std::make_tuple(x, eval_expr(x->lhs)->clone(), eval_expr(x->rhs));
    }

  private:
    CallStack& cur_stack() { return *call_stack.back(); }
  };
} // namespace superman