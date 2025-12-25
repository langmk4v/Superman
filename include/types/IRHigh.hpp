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

#include "Node.hpp"

namespace fire {

  enum class HIRKind {
    Expr,
    Stmt,
    Function,
    Struct,
    Enum,
  };

  enum class HIRStmtKind {
    Scope,
    Vardef,
    If,
    While,
    Loop,
    Break,
    Continue,
    Return,
    TryCatch,
    Expr,
  };

  struct HIR {
    HIRKind kind;

    void dump() const;

  protected:
    HIR(HIRKind kind) : kind(kind) {}
  };

  struct HIRExpr : HIR {
    Node* expr = nullptr;
    TypeInfo type;

    HIRExpr(Node* expr, TypeInfo type) : HIR(HIRKind::Expr), expr(expr), type(std::move(type)) {}
  };

  struct HIRStmt : HIR {
    HIRStmtKind kind;

  protected:
    HIRStmt(HIRStmtKind kind) : HIR(HIRKind::Stmt), kind(kind) {}
  };

  struct HIRExprStmt : HIRStmt {
    HIRExpr* expr = nullptr;
    HIRExprStmt(HIRExpr* expr) : HIRStmt(HIRStmtKind::Expr), expr(expr) {}
  };

  struct HIRScope : HIRStmt {
    std::vector<HIRStmt*> items;
    HIRScope(std::vector<HIRStmt*> items) : HIRStmt(HIRStmtKind::Scope), items(items) {}
  };

  struct HIRVardef : HIRStmt {
    std::string name;
    HIRExpr* expr = nullptr;
    HIRVardef(std::string const& name, HIRExpr* expr) : HIRStmt(HIRStmtKind::Vardef), name(name), expr(expr) {}
  };

  struct HIRIf : HIRStmt {
    HIRExpr* cond = nullptr;
    HIRStmt* then_stmt = nullptr;
    HIRStmt* else_stmt = nullptr;
    HIRIf(HIRExpr* cond, HIRStmt* then_stmt, HIRStmt* else_stmt)
        : HIRStmt(HIRStmtKind::If), cond(cond), then_stmt(then_stmt), else_stmt(else_stmt) {}
  };

  struct HIRLoop : HIRStmt {
    HIRScope* body = nullptr;
    HIRLoop(HIRScope* body) : HIRStmt(HIRStmtKind::Loop), body(body) {}
  };

  struct HIRBreak : HIRStmt {
    HIRBreak() : HIRStmt(HIRStmtKind::Break) {}
  };

  struct HIRContinue : HIRStmt {
    HIRContinue() : HIRStmt(HIRStmtKind::Continue) {}
  };

  struct HIRReturn : HIRStmt {
    HIRExpr* expr = nullptr;
    HIRReturn(HIRExpr* expr) : HIRStmt(HIRStmtKind::Return), expr(expr) {}
  };

  struct HIRTryCatch : HIRStmt {
    struct Catch {
      std::string holder_name;
      TypeInfo holder_type;
      HIRScope* body = nullptr;
    };

    HIRScope* body = nullptr;
    std::vector<Catch> catches;
    HIRScope* finally_block = nullptr;

    HIRTryCatch(HIRScope* body, std::vector<Catch> catches, HIRScope* finally_block)
        : HIRStmt(HIRStmtKind::TryCatch), body(body), catches(std::move(catches)), finally_block(finally_block) {}
  };

  struct HIRFunction : HIR {
    std::string name;
    HIRScope* body = nullptr;
    TypeInfo result_type;
    HIRFunction(std::string const& name, HIRScope* body, TypeInfo result_type)
        : HIR(HIRKind::Function), name(name), body(body), result_type(std::move(result_type)) {}
  };

  struct HIRStruct : HIR {
    struct Field {
      std::string name;
      TypeInfo type;
    };

    std::string name;
    std::vector<Field> fields;

    HIRStruct(std::string const& name, std::vector<Field> fields)
        : HIR(HIRKind::Struct), name(name), fields(std::move(fields)) {}
  };

  struct HIREnum : HIR {
    std::string name;
    std::vector<std::string> enumerators;
    HIREnum(std::string const& name, std::vector<std::string> enumerators)
        : HIR(HIRKind::Enum), name(name), enumerators(std::move(enumerators)) {}
  };

} // namespace fire