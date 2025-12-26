#pragma once

#include "Node.hpp"
#include "IRHigh.hpp"
#include "IRMiddle.hpp"
#include "IRLow.hpp"

namespace fire {
  class HighIRCreator {
  public:
    static HIR* create_full_hir(Node* node);
  };

  class MiddleIRCreator {
  public:
    static MIR* create_full_mir(Node* node);
  };

  class LowIRCreator {
  public:
    static LIR* create_full_lir(Node* node);
  };

  class NodeLower {
  public:
    static LIR* lower_full(Node* node);
  };
} // namespace fire