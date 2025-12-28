#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "Node.hpp"
#include "Object.hpp"
#include "TypeInfo.hpp"

namespace fire {

struct NodeEvaluatorConfig {
  size_t max_stack_depth = 512;
};

class NodeEvaluator {
  using VariablesMap = std::unordered_map<std::string, Object*>;

  struct ScopeStack {
    VariablesMap variables;
  };

  struct CallStack {
    Object* func_result = nullptr;
    std::vector<std::shared_ptr<ScopeStack>> scope_stack;
  };

  VariablesMap global_variables;

  std::vector<std::shared_ptr<CallStack>> call_stack;

  NodeEvaluatorConfig const config;

public:
  NodeEvaluator(NodeEvaluatorConfig config);

  Object*& eval_lvalue(Node* node);
  Object* eval_expr(Node* node);

  void eval_stmt(Node* node);
  void eval_scope(NdScope* node);

  std::shared_ptr<CallStack> push_call_stack();
  void pop_call_stack();
  CallStack& current_stack();

  std::shared_ptr<ScopeStack> push_scope_stack();
  void pop_scope_stack();
  ScopeStack& current_scope_stack();
};
} // namespace fire