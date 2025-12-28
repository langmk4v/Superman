#pragma once

#include <concepts>
#include <functional>

#include "Node.hpp"

namespace fire {

template <typename VisitorFuncResult, bool CallAtTop, bool CallAtLast>
class NodeVisitor {
public:
  enum Locations {
    Top,
    Main,
    Last,
  };

  VisitorFuncResult
  visit(Node* node,
        std::function<bool(Node*, Locations, VisitorFuncResult&)> f) {
    VisitorFuncResult result;

    if (!node) return result;

    if constexpr (CallAtTop) {
      if (!f(node, Top, result)) return result;
    }

    if (!f(node, Main, result)) return result;

    switch (node->kind) {
    case NodeKind::Value:
    case NodeKind::Self:
    case NodeKind::NullOpt:
      break;

    case NodeKind::KeyValuePair:
      if (!visit(node->as<NdKeyValuePair>()->key, f)) break;
      if (!visit(node->as<NdKeyValuePair>()->value, f)) break;
      break;

    case NodeKind::DeclType:
      if (!visit(node->as<NdDeclType>()->type, f)) break;
      break;

    case NodeKind::Symbol:
      if (!visit(node->as<NdSymbol>()->dec, f)) break;
      if (!visit(node->as<NdSymbol>()->next, f)) break;
      for (auto te : node->as<NdSymbol>()->te_args) {
        if (!visit(te, f)) break;
      }
      break;

    case NodeKind::Array:
      for (auto elem : node->as<NdArray>()->data) {
        if (!visit(elem, f)) break;
      }
      break;

    case NodeKind::Tuple:
      for (auto elem : node->as<NdTuple>()->elems) {
        if (!visit(elem, f)) break;
      }
      break;

    case NodeKind::CallFunc:
      if (!visit(node->as<NdCallFunc>()->callee, f)) break;
      if (!visit(node->as<NdCallFunc>()->inst_expr, f)) break;
      for (auto arg : node->as<NdCallFunc>()->args) {
        if (!visit(arg, f)) break;
      }
      break;

    case NodeKind::GetTupleElement:
      if (!visit(node->as<NdGetTupleElement>()->expr, f)) break;
      break;

    case NodeKind::Inclement:
    case NodeKind::Declement:
      if (!visit(node->as<NdInclementDeclement>()->expr, f)) break;
      break;

    case NodeKind::BitNot:
    case NodeKind::Not:
    case NodeKind::Ref:
    case NodeKind::Deref:
      if (!visit(node->as<NdOneExprWrap>()->expr, f)) break;
      break;

    case NodeKind::Mul:
    case NodeKind::Div:
    case NodeKind::Mod:
    case NodeKind::Add:
    case NodeKind::Sub:
    case NodeKind::LShift:
    case NodeKind::RShift:
    case NodeKind::BitAnd:
    case NodeKind::BitXor:
    case NodeKind::BitOr:
    case NodeKind::LogAnd:
    case NodeKind::LogOr:
    case NodeKind::Equal:
    case NodeKind::Bigger:
    case NodeKind::BiggerOrEqual:
    case NodeKind::Assign:
    case NodeKind::AssignWithOp:
      if (!visit(node->as<NdExpr>()->lhs, f)) break;
      if (!visit(node->as<NdExpr>()->rhs, f)) break;
      break;

    case NodeKind::Scope:
      for (auto item : node->as<NdScope>()->items) {
        if (!visit(item, f)) break;
      }
      break;

    case NodeKind::Let:
      if (!visit(node->as<NdLet>()->type, f)) break;
      if (!visit(node->as<NdLet>()->init, f)) break;
      break;

    case NodeKind::Try:
      todo;

    case NodeKind::Catch:
      todo;

    default:
      todo;
    }

    if constexpr (CallAtLast) {
      return f(node, Last);
    }

    return result;
  }
};

} // namespace fire