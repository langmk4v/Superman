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

    std::vector<CallStack> call_stack;

  public:
    Evaluator() {}

    Object* eval_node(Node* node) {
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

  private:
  };
} // namespace superman