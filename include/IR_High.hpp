/*

## IR_High: 中間表現 (高級)

## 以下の構造を削除する
- namespace
- class ( --> struct + function )

## 文を簡略化する
- for, while, ...     --> loop
- if, match, switch   --> if

*/

#pragma once

#include "Object.hpp"

namespace fire {

  struct Node;

  enum class HIRKind {
    Expr,
    Stmt,
    Function,
    Struct,
    Enum,
  };

  enum class HIRStmtKind {
    Scope,
    If,
    While,
    Loop,
    Break,
    Return,
    Expr,
  };

  struct HIR {
    HIRKind kind;

    static HIR* from_node(Node* nd);
  }

} // namespace fire