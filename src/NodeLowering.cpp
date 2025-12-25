#include "NodeLowering.hpp"

namespace fire {

  HIRExpr* NodeLowering::make_expr_hir(Node* node, TypeInfo type) {
    return new HIRExpr(node, type);
  }

} // namespace fire