/*

NodeLowering:
  コンパイル直前段階の中間表現 (IR)

  クラスや名前空間を削除して関数と型定義だけにされた状態
  new T(...) などのインスタンス作成は、T::new(T(), ...) に変換

*/

#pragma once

#include "Parser/Node.hpp"
#include "VM/Interp/Object.hpp"

namespace fire::vm {

  enum LowNodeKind {
    ND_Value,

    ND_GetVar,
    ND_GetArg,

    ND_CallFunc,

    ND_Make, // make instance of type

    ND_Expr, // <- use NodeKind for operator

    ND_Vardef,
    ND_Return,

    ND_FuncDef,
    ND_TypeDef,  // define a struct
  };

  struct LowNode {
    LowNodeKind kind;

  protected:
    LowNode(LowNodeKind kind) : kind(kind) { }
  };

  struct LnValue : LowNode {
    vm::interp::Object* value = nullptr;

    LnValue(parser::NdValue* val) : LowNode(ND_Value), value(val->obj) { }
  };

  struct LnGet : LowNode {
    size_t index = 0;
    bool is_arg = false;

    LnGet(parser::NdSymbol* nd) : LowNode(ND_GetVar), index(nd->var_offset), is_arg(!nd->is_var()) { }
  };

  struct LnCallFunc : LowNode {
    std::string name;
    std::vector<LowNode*> args;

    LnCallFunc(std::string const& name, std::vector<LowNode*> args)
      : LowNode(ND_CallFunc), name(name), args(std::move(args)) { }
  };


}