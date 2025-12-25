#include "IR.hpp"
#include "Utils.hpp"

namespace fire {

  IR* IR::from_node(Node* nd) {
    switch (nd->kind) {
    case NodeKind::Value:
      return new IRValue(nd->as<NdValue>()->obj);

    case NodeKind::Module: {
    }
    }

    todo;
  }

} // namespace fire