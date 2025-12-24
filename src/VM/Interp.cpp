#include "Utils/macro.h"
#include "Utils/Strings.hpp"

#include "Sema/Sema.hpp"

#include "VM/Instruction.hpp"
#include "VM/Compiler.hpp"

#include "VM/Interp/Interp.hpp"
#include "VM/Interp/Object.hpp"

namespace fire {

using namespace lexer;
using namespace parser;
using namespace sema;

using vm::Instruction;

namespace vm::interp {

  static auto get_expr_tu(Interp* i, Node* node) {
    auto x = node->as<NdExpr>();
    return std::make_tuple(x, i->eval_expr(x->lhs), i->eval_expr(x->rhs));
  }

  Interp::Interp(std::vector<Instruction>& prg)
    : prg(prg)
  {
  }

  Object* Interp::eval_expr(Node* node) {

    assert(node != nullptr);

    switch (node->kind) {
    case NodeKind::Value:
      return node->as<NdValue>()->obj;

    case NodeKind::Symbol: {
      auto sym = node->as<NdSymbol>();

      if(sym->is_local_var){
        todo;
      }

      todo;
    }

    //
    // call-func expression
    //
    case NodeKind::CallFunc: {
      auto cf = node->as<NdCallFunc>();

      (void)cf;

      std::vector<Object*> args;

      for (auto&& a : cf->args)
        args.push_back(eval_expr(a));

      // user-defined
      if (cf->func_nd) {
        todo;
      }

      // built-in
      todoimpl;
    }

    //
    // operator "new"
    //
    case NodeKind::New: {
      auto x = node->as<NdNew>();

      todo;
    }

      using Ty = TypeKind;

    case NodeKind::Add: {
      auto [x, l, r] = get_expr_tu(this, node);

      switch (l->type.kind) {
      case Ty::Int:
        l->as<ObjInt>()->val += r->as<ObjInt>()->val;
        break;
      case Ty::Float:
        l->as<ObjFloat>()->val += r->as<ObjFloat>()->val;
        break;
      case Ty::String:
        l->as<ObjString>()->val += r->as<ObjString>()->val;
        break;
      }

      return l;
    }

    case NodeKind::Sub: {
      auto [x, l, r] = get_expr_tu(this, node);

      switch (l->type.kind) {
      case Ty::Int:
        l->as<ObjInt>()->val -= r->as<ObjInt>()->val;
        break;
      case Ty::Float:
        l->as<ObjFloat>()->val -= r->as<ObjFloat>()->val;
        break;
      }

      return l;
    }

    case NodeKind::Mul: {
      auto [x, l, r] = get_expr_tu(this, node);

      // Handle string * number
      if (l->type.kind == Ty::String && (r->type.kind == Ty::Int || r->type.kind == Ty::Float)) {
        auto str = l->as<ObjString>();
        int count = (r->type.kind == Ty::Int) ? static_cast<int>(r->as<ObjInt>()->val)
                                              : static_cast<int>(r->as<ObjFloat>()->val);

        std::u16string result;
        for (int i = 0; i < count; ++i) {
          result += str->val;
        }
        str->val = result;
        return l;
      }
      // Handle number * string
      else if ((l->type.kind == Ty::Int || l->type.kind == Ty::Float) &&
               r->type.kind == Ty::String) {
        auto str = r->as<ObjString>();
        int count = (l->type.kind == Ty::Int) ? static_cast<int>(l->as<ObjInt>()->val)
                                              : static_cast<int>(l->as<ObjFloat>()->val);

        std::u16string result;
        for (int i = 0; i < count; ++i) {
          result += str->val;
        }
        str->val = result;
        return r;
      }
      // Handle number * number
      else if (l->type.kind == Ty::Int && r->type.kind == Ty::Int) {
        l->as<ObjInt>()->val *= r->as<ObjInt>()->val;
      } else if (l->type.kind == Ty::Float || r->type.kind == Ty::Float) {
        double lval = (l->type.kind == Ty::Int) ? l->as<ObjInt>()->val : l->as<ObjFloat>()->val;
        double rval = (r->type.kind == Ty::Int) ? r->as<ObjInt>()->val : r->as<ObjFloat>()->val;

        if (l->type.kind == Ty::Int) {
          l = new ObjFloat(lval * rval);
        } else {
          l->as<ObjFloat>()->val = lval * rval;
        }
      } else {
        // Type error - should be handled by type checker
        todoimpl;
      }

      return l;
    }

    default:
      todoimpl;
    }

    return Object::none;
  }

  void Interp::run() {
    (void)prg;

  }

}

}