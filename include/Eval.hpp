#pragma once

#include <unordered_map>

#include "macro.h"

#include "Node.hpp"
#include "Object.hpp"

namespace superman {
  class Evaluator {

    struct CallStack {
      std::unordered_map<std::string, Object*> variables;
    };

    std::vector<CallStack*> call_stack;

  public:
    Evaluator() {}

    Object* eval_node(Node* node);

    CallStack& push_stack();

    void pop_stack();

  private:
  };
} // namespace superman