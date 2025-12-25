#pragma once

#include <string>
#include <vector>

#include "TypeInfo.hpp"
#include "Node.hpp"

namespace fire {

  enum class SymbolKind {
    Unknown,
    Var,
    Func,
    Enum,
    Enumerator,
    Class,
    Namespace,
    Module,
  };

  struct VariableInfo;
  struct Scope;

  struct Symbol {
    std::string name = "";
    SymbolKind kind = SymbolKind::Unknown;
    TypeInfo type = {};
    Node* node = nullptr;
    Token* token = nullptr;
    VariableInfo* var_info = nullptr;
    Scope* scope = nullptr;
  };

  struct SymbolTable {
    SymbolTable* parent = nullptr;

    std::vector<Symbol*> symbols;

    SymbolTable(SymbolTable* parent = nullptr) : parent(parent) {
    }

    Symbol*& append(Symbol* s) {
      return symbols.emplace_back(s);
    }
  };

} // namespace fire