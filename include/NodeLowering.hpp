#pragma once

#include "Node.hpp"
#include "IRHigh.hpp"

namespace fire {
  class NodeLowering {
  public:
    static HIRExpr* make_expr_hir(Node* node, TypeInfo type);
  };
} // namespace fire