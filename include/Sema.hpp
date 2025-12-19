#pragma once

#include <set>

#include "Node.hpp"
#include "TypeInfo.hpp"

//
// スコープコンテキストを主軸として処理をまわす
//

namespace superman::sema {
  enum class SymbolKind {
    Var,
    Func,
    Enum,
    Enumerator,
    Class,
    Module,
    TemplateParam,
  };

  struct ScopeContext;
  struct VariableInfo;

  //
  // Symbol
  //
  struct Symbol {
    SymbolKind kind;

    std::string name;

    Node* node = nullptr;

    ScopeContext* scope_ctx = nullptr;

    VariableInfo* var_info = nullptr; // if variable

    std::set<Node*> references;

    bool has_scope() const {
      return scope_ctx != nullptr;
    }

    bool is(SymbolKind k) const {
      return kind == k;
    }

    bool is_type_name() const {
      return is(SymbolKind::Enum) || is(SymbolKind::Class) || is(SymbolKind::TemplateParam);
    }

    static Symbol* new_var_symbol(NdLet* let);

    static Symbol* new_arg_symbol(NdFunction::Argument* arg);

    Symbol(SymbolKind k, std::string const& n, Node* node) : kind(k), name(n), node(node) {
    }
  };

  //
  // SymbolTable
  //
  struct SymbolTable {
    SymbolTable* parent_tbl;

    std::vector<Symbol*> symbols;

    Symbol*& append(Symbol* s) {
      return symbols.emplace_back(s);
    }

    // iterators
    auto begin() {
      return symbols.begin();
    }
    auto end() {
      return symbols.end();
    }

    SymbolTable(SymbolTable* parent_tbl = nullptr) : parent_tbl(parent_tbl) {
    }
  };

  struct VariableInfo {
    bool is_type_deducted = false;
    TypeInfo type;

    NdLet* def_let = nullptr;
    NdFunction::Argument* def_arg = nullptr;

    std::set<NdExpr*> assignments;

    VariableInfo() {
    }
  };

  struct ScopeContext {
    Node* node;

    template <typename T>
    T* as() {
      return (T*)this;
    }

  protected:
    ScopeContext(Node* node) : node(node) {
    }
  };

  // UnnamedScope
  // 関数スコープ内の無名スコープ (NdScope)
  struct UnnamedScope : ScopeContext {
    SymbolTable variables;

    std::vector<UnnamedScope*> subscopes;

    UnnamedScope* add_scope(UnnamedScope* us) {
      return subscopes.emplace_back(us);
    }

    UnnamedScope(NdScope* scope);
  };

  //
  // 関数スコープ
  struct FunctionScope : ScopeContext {
    SymbolTable args;
    UnnamedScope* body = nullptr;

    FunctionScope(NdFunction* func);
  };

  struct EnumScope : ScopeContext {};

  struct ClassScope : ScopeContext {};

  struct ModuleScope : ScopeContext {
    SymbolTable variables;
    SymbolTable functions;
    SymbolTable enums;
    SymbolTable classes;

    ModuleScope(NdModule* mod);
  };

  struct SymbolFindResult {

    std::vector<Symbol*> v;

    bool nothing() const {
      return v.empty();
    }

    auto count() {
      return v.size();
    }

    Symbol* operator[](size_t i) {
      return v[i];
    }

    void remove(size_t at) {
      v.erase(v.begin() + at);
    }
  };

  //
  // ExprTypeResult
  // 式の型を評価した結果を表す構造体
  struct ExprTypeResult {
    TypeInfo type;

    bool is_succeed = false;

    // 式中に含まれる要素の型に依存してる場合は true
    bool is_type_dependent = false;
    std::vector<Node*> depends;

    bool fail() const {
      return !is_succeed;
    }

    ExprTypeResult() {
    }

    ExprTypeResult(TypeInfo t) : type(std::move(t)) {
      is_succeed = true;
    }
  };

  class Sema {
    ModuleScope* root_scope = nullptr;

    ScopeContext* cur_scope = nullptr;

  public:
    Sema(NdModule* mod);

    void analyze_full();

    void check_module(ModuleScope* mod);
    void check_function(FunctionScope* func);
    void check_scope(UnnamedScope* scope);

    ExprTypeResult eval_expr(Node* node);

    ExprTypeResult eval_typename(NdSymbol* node);

    SymbolFindResult find_symbol(NdSymbol* node);

    static int get_required_template_params_count(Symbol* s);
  };
} // namespace superman::sema