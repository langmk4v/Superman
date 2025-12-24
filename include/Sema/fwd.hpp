#pragma once

namespace fire::sema {
  struct Symbol;
  struct SymbolTable;
  struct VariableInfo;

  struct ScopeContext;
  struct UnnamedScope;
  struct FunctionScope;
  struct EnumScope;
  struct ClassScope;
  struct ModuleScope;

  struct SymbolFindResult;
  struct ExprType;

  class Sema;
}