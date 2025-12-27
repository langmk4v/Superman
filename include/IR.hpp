/*

## IR_High: 中間表現 (高級)

## 以下の構造を削除する
  - namespace
  - class ( --> struct + function )

## 文を簡略化する
  - for, while, ...     --> loop
  - if, match, switch   --> if

  ** try-catch はまだ残す **

*/

#pragma once

#include "Node.hpp"

namespace fire::IR::High {
  enum class Kind {
    Expr,
    Stmt,
    Function,
    Struct,
    Enum,
  };

  enum class StmtKind {
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

  struct Base {
    Kind kind;

    void dump() const;

  protected:
    Base(Kind kind) : kind(kind) {}
  };

  struct IRExpr : Base {
    Node* expr = nullptr;
    TypeInfo type;

    IRExpr(Node* expr, TypeInfo type) : Base(Kind::Expr), expr(expr), type(std::move(type)) {}
  };

  struct IRStmt : Base {
    StmtKind kind;

  protected:
    IRStmt(StmtKind kind) : Base(Kind::Stmt), kind(kind) {}
  };

  struct IRExprStmt : IRStmt {
    IRExpr* expr = nullptr;
    IRExprStmt(IRExpr* expr) : IRStmt(StmtKind::Expr), expr(expr) {}
  };

  struct IRScope : IRStmt {
    std::vector<IRStmt*> items;
    IRScope(std::vector<IRStmt*> items) : IRStmt(StmtKind::Scope), items(items) {}
  };

  struct IRVardef : IRStmt {
    std::string name;
    IRExpr* expr = nullptr;
    IRVardef(std::string const& name, IRExpr* expr)
        : IRStmt(StmtKind::Vardef), name(name), expr(expr) {}
  };

  struct IRIf : IRStmt {
    IRExpr* cond = nullptr;
    IRStmt* then_stmt = nullptr;
    IRStmt* else_stmt = nullptr;
    IRIf(IRExpr* cond, IRStmt* then_stmt, IRStmt* else_stmt)
        : IRStmt(StmtKind::If), cond(cond), then_stmt(then_stmt), else_stmt(else_stmt) {}
  };

  struct IRLoop : IRStmt {
    IRScope* body = nullptr;
    IRLoop(IRScope* body) : IRStmt(StmtKind::Loop), body(body) {}
  };

  struct IRBreak : IRStmt {
    IRBreak() : IRStmt(StmtKind::Break) {}
  };

  struct IRContinue : IRStmt {
    IRContinue() : IRStmt(StmtKind::Continue) {}
  };

  struct IRReturn : IRStmt {
    IRExpr* expr = nullptr;
    IRReturn(IRExpr* expr) : IRStmt(StmtKind::Return), expr(expr) {}
  };

  struct IRTryCatch : IRStmt {
    struct Catch {
      std::string holder_name;
      TypeInfo holder_type;
      IRScope* body = nullptr;
    };

    IRScope* body = nullptr;
    std::vector<Catch> catches;
    IRScope* finally_block = nullptr;

    IRTryCatch(IRScope* body, std::vector<Catch> catches, IRScope* finally_block)
        : IRStmt(StmtKind::TryCatch), body(body), catches(std::move(catches)),
          finally_block(finally_block) {}
  };

  struct IRFunction : Base {
    std::string name;
    IRScope* body = nullptr;
    TypeInfo result_type;
    IRFunction(std::string const& name, IRScope* body, TypeInfo result_type)
        : Base(Kind::Function), name(name), body(body), result_type(std::move(result_type)) {}
  };

  struct IRStruct : Base {
    struct Field {
      std::string name;
      TypeInfo type;
    };

    std::string name;
    std::vector<Field> fields;

    IRStruct(std::string const& name, std::vector<Field> fields)
        : Base(Kind::Struct), name(name), fields(std::move(fields)) {}
  };

  struct IREnum : Base {
    std::string name;
    std::vector<std::string> enumerators;
    IREnum(std::string const& name, std::vector<std::string> enumerators)
        : Base(Kind::Enum), name(name), enumerators(std::move(enumerators)) {}
  };
} // namespace fire::IR::High

namespace fire::IR::Middle {
  struct MIR {};
} // namespace fire::IR::Middle

namespace fire::IR::Low {
  struct LIR {};
} // namespace fire::IR::Low