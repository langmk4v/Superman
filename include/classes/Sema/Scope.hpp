#pragma once

#include "Utils.hpp"
#include "Symbols.hpp"

namespace fire {

  enum class ScopeKind {
    Scope,
    Try,
    Catch,
    If,
    For,
    Func,
    Enum,
    Class,
    Namespace,
    Module,
  };

  struct Scope {
    ScopeKind kind;

    Node* node = nullptr;
    Scope* parent = nullptr;

    Symbol symbol;
    SymbolTable symtable;

    template <typename T>
    T* as() {
      return static_cast<T*>(this);
    }

    virtual ~Scope() = default;

  protected:
    Scope(ScopeKind kind, Node* node, Scope* parent) : kind(kind), node(node), parent(parent) {
    }
  };

  struct SCScope : Scope {
    SymbolTable variables;
    std::vector<Scope*> subscopes;

    SCScope(NdScope* node, Scope* parent) : Scope(ScopeKind::Scope, node, parent) {
    }
  };

  struct SCIf : Scope {
    Symbol* var = nullptr;
    SCScope* then_scope = nullptr;
    SCScope* else_scope = nullptr;

    SCIf(NdIf* node, Scope* parent) : Scope(ScopeKind::If, node, parent) {
    }
  };

  struct SCFor : Scope {
    Symbol* iter_name = nullptr;
    SCScope* body = nullptr;

    SCFor(NdFor* node, Scope* parent) : Scope(ScopeKind::For, node, parent) {
    }
  };

  struct SCCatch : Scope {
    Symbol* holder_name = nullptr;
    SCScope* body = nullptr;

    SCCatch(NdCatch* node, Scope* parent) : Scope(ScopeKind::Catch, node, parent) {
    }
  };

  struct SCTry : Scope {
    SCScope* body = nullptr;
    std::vector<SCCatch*> catches;
    SCScope* finally_scope = nullptr;

    SCTry(NdTry* node, Scope* parent) : Scope(ScopeKind::Try, node, parent) {
    }
  };

  struct SCFunction : Scope {
    SymbolTable arguments;
    SCScope* body = nullptr;

    SCFunction(NdFunction* node, Scope* parent) : Scope(ScopeKind::Func, node, parent) {
    }
  };

  struct SCEnum : Scope {
    SymbolTable enumerators;
    SCEnum(NdEnum* node, Scope* parent) : Scope(ScopeKind::Enum, node, parent) {
    }
  };

  struct SCClass : Scope {
    SymbolTable fields;
    SymbolTable methods;

    SCClass(NdClass* node, Scope* parent) : Scope(ScopeKind::Class, node, parent) {
    }
  };

  struct SCNamespace : Scope {
    SymbolTable variables;
    SymbolTable functions;
    SymbolTable enums;
    SymbolTable classes;
    SymbolTable namespaces;

    SymbolTable* get_table(NodeKind kind) {
      switch (kind) {
        case NodeKind::Let:
          return &variables;
        case NodeKind::Function:
          return &functions;
        case NodeKind::Enum:
          return &enums;
        case NodeKind::Class:
          return &classes;
        case NodeKind::Namespace:
          return &namespaces;
      }
      todo;
    }

    SCNamespace(NdNamespace* node, Scope* parent) : Scope(ScopeKind::Namespace, node, parent) {
    }
  };

  struct SCModule : Scope {
    SymbolTable variables;
    SymbolTable functions;
    SymbolTable enums;
    SymbolTable classes;
    SymbolTable namespaces;

    SymbolTable* get_table(NodeKind kind) {
      switch (kind) {
        case NodeKind::Let:
          return &variables;
        case NodeKind::Function:
          return &functions;
        case NodeKind::Enum:
          return &enums;
        case NodeKind::Class:
          return &classes;
        case NodeKind::Namespace:
          return &namespaces;
      }
      todo;
    }

    SCModule(NdModule* node, Scope* parent) : Scope(ScopeKind::Module, node, parent) {
    }
  };

} // namespace fire