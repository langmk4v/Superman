/*

NodeLowering:
  コンパイル直前段階の中間表現 (IR)

  クラスや名前空間を削除して関数と型定義だけにされた状態
  new T(...) などのインスタンス作成は、T::new(T(), ...) に変換

*/

#pragma once

#include "Node.hpp"
#include "Object.hpp"

namespace fire {
  enum class IRKind {
    //
    // value
    // - 123
    Value,

    //
    // function call
    // - func(...)
    CallFunc,

    //
    // instance creation
    // - T(...)
    MakeInstance,

    Expr,

    Scope,

    Vardef,

    If,
    For,
    While,

    Return,

    Function,

    Enum,
    Struct,
  };

  struct IR {
    IRKind kind;

    static IR* from_node(Node* nd);

  protected:
    IR(IRKind kind) : kind(kind) {}
  };

  struct IRExprBase : IR {
  protected:
    IRExprBase(IRKind kind) : IR(kind) {}
  };

  struct IRValue : IRExprBase {
    Object* obj;

    IRValue(Object* obj) : IRExprBase(IRKind::Value), obj(obj) {}
  };

  struct IRCallFunc : IRExprBase {
    IRExprBase* callee;
    std::vector<IRExprBase*> args;

    IRCallFunc(IRExprBase* callee, std::vector<IRExprBase*> args)
        : IRExprBase(IRKind::CallFunc), callee(callee), args(std::move(args)) {}
  };

  struct IRExpr : IRExprBase {
    NodeKind expr_kind;
    std::vector<IRExprBase*> terms;

    IRExpr(Node* expr, std::vector<IRExprBase*> terms)
        : IRExprBase(IRKind::Expr), expr_kind(expr->kind), terms(std::move(terms)) {}
  };

  struct IRStmtBase : IR {
  protected:
    IRStmtBase(IRKind kind) : IR(kind) {}
  };

  struct IRScope : IRStmtBase {
    std::vector<IRStmtBase*> items;

    IRScope(std::vector<IRStmtBase*> items) : IRStmtBase(IRKind::Scope), items(std::move(items)) {}
  };

  struct IRVardef : IRStmtBase {
    std::string name;
    IRExprBase* init;

    IRVardef(std::string name, IRExprBase* init) : IRStmtBase(IRKind::Vardef), name(std::move(name)), init(init) {}
  };

  struct IRFunction : IRStmtBase {
    std::string name;
    std::vector<std::string> arg_names;
    IRScope* body;

    IRFunction(std::string name, std::vector<std::string> arg_names, IRScope* body)
        : IRStmtBase(IRKind::Function), name(std::move(name)), arg_names(std::move(arg_names)), body(body) {}
  };
} // namespace fire