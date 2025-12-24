#pragma once

#include <set>
#include <vector>

#include "Parser/Node.hpp"
#include "Sema/TypeInfo.hpp"

//
// スコープコンテキストを主軸として処理をまわす
//

namespace fire::sema {
  using namespace lexer;
  using namespace parser;

  enum class SymbolKind {
    Var,
    Func,
    Method,
    Enum,
    Enumerator,
    Class,
    Module,
    TemplateParam,
  };

  struct ScopeContext;
  struct UnnamedScope;
  struct FunctionScope;
  struct EnumScope;
  struct ClassScope;
  struct ModuleScope;

  struct VariableInfo;

  //
  // Symbol
  //
  struct Symbol {
    SymbolKind kind;

    std::string name;

    Node* node = nullptr;

    union {
      ScopeContext* scope_ctx = nullptr;

      UnnamedScope* unnscope;
      FunctionScope* fnscope;
      EnumScope* enscope;
      ClassScope* clscope;
      ModuleScope* modscope;
    };

    VariableInfo* var_info = nullptr; // if variable

    std::set<Node*> references;

    bool has_scope() const { return scope_ctx != nullptr; }

    bool is(SymbolKind k) const { return kind == k; }

    bool is_type_name() const {
      return is(SymbolKind::Enum) || is(SymbolKind::Class) || is(SymbolKind::TemplateParam);
    }

    //
    // new_var_symbol
    //  ローカル変数またはグローバル変数、メンバ変数用のシンボル情報を作成
    static Symbol* new_var_symbol(NdLet* let, bool is_field = false);

    //
    // new_arg_symbol
    //  関数の引数用のシンボル情報を作成
    static Symbol* new_arg_symbol(NdFunction::Argument* arg);

    Symbol(SymbolKind k, std::string const& n, Node* node) : kind(k), name(n), node(node) {}
  };

  //
  // SymbolTable
  //
  struct SymbolTable {
    SymbolTable* parent_tbl;

    std::vector<Symbol*> symbols;

    Symbol*& append(Symbol* s) { return symbols.emplace_back(s); }

    // iterators
    auto begin() { return symbols.begin(); }
    auto begin() const { return symbols.begin(); }
    auto end() { return symbols.end(); }
    auto end() const { return symbols.end(); }

    auto size() const { return symbols.size(); }

    SymbolTable(SymbolTable* parent_tbl = nullptr) : parent_tbl(parent_tbl) {}
  };
  
  struct VariableInfo {
    bool is_type_deducted = false;
    TypeInfo type;

    int offset = 0;

    bool is_global = false;

    bool is_field = false;

    NdLet* def_let = nullptr;
    NdFunction::Argument* def_arg = nullptr;

    std::set<Node*> assignments; // <-- Assign or Let

    bool is_argument() const { return def_arg != nullptr; }

    bool is_local_v() const { return !is_global && !is_field && !is_argument(); }

    VariableInfo() {}
  };
}