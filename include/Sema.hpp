#pragma once

#include <set>

#include "Node.hpp"
#include "TypeInfo.hpp"
#include "BuiltinFunc.hpp"

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

    bool has_scope() const { return scope_ctx != nullptr; }

    bool is(SymbolKind k) const { return kind == k; }

    bool is_type_name() const {
      return is(SymbolKind::Enum) || is(SymbolKind::Class) || is(SymbolKind::TemplateParam);
    }

    static Symbol* new_var_symbol(NdLet* let);

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

    NdLet* def_let = nullptr;
    NdFunction::Argument* def_arg = nullptr;

    std::set<Node*> assignments; // <-- Assign or Let

    VariableInfo() {}
  };

  struct ScopeContext {
    Node* node;

    ScopeContext* parent = nullptr;

    template <typename T>
    T* as() {
      return (T*)this;
    }

    bool is(NodeKind k) const { return node->kind == k; }

    virtual size_t find_symbol(std::vector<Symbol*>&, std::string const&) const { return 0; }

  protected:
    ScopeContext(Node* node, ScopeContext* parent = nullptr) : node(node), parent(parent) {}
  };

  struct FunctionScope;

  // UnnamedScope
  // 関数スコープ内の無名スコープ (NdScope)
  struct UnnamedScope : ScopeContext {
    SymbolTable variables;

    std::vector<UnnamedScope*> subscopes;

    FunctionScope* parent_fn = nullptr;

    UnnamedScope* add_scope(UnnamedScope* us) {
      us->parent = this;
      return subscopes.emplace_back(us);
    }

    size_t find_symbol(std::vector<Symbol*>& out, std::string const& name) const override {
      for (auto&& v : variables)
        if (v->name == name) out.push_back(v);

      return out.size();
    }

    UnnamedScope(NdScope* scope, int var_offset, FunctionScope* fs);
  };

  //
  // 関数スコープ
  struct FunctionScope : ScopeContext {
    SymbolTable args;
    TypeInfo result_type;
    UnnamedScope* body = nullptr;

    int local_var_count = 0;

    // 戻り値の型の管理フラグ
    // ・プログラムにて明示的に指定されている場合は specified = true
    // ・そうでない場合に return 文から推論できた場合は deducted = true
    bool is_result_type_specified = false;
    bool is_result_type_deducted = false; // *両方が true になることはない

    size_t find_symbol(std::vector<Symbol*>& out, std::string const& name) const override {
      for (auto&& v : args)
        if (v->name == name) out.push_back(v);

      return out.size();
    }

    FunctionScope(NdFunction* func);
  };

  struct EnumScope : ScopeContext {};

  struct ClassScope : ScopeContext {};

  struct ModuleScope : ScopeContext {
    SymbolTable variables;
    SymbolTable functions;
    SymbolTable enums;
    SymbolTable classes;

    size_t find_symbol(std::vector<Symbol*>& out, std::string const& name) const override {
      for (auto&& v : variables)
        if (v->name == name) out.push_back(v);

      for (auto&& v : functions)
        if (v->name == name) out.push_back(v);

      for (auto&& v : enums)
        if (v->name == name) out.push_back(v);

      for (auto&& v : classes)
        if (v->name == name) out.push_back(v);

      return out.size();
    }

    size_t find_variable(std::vector<Symbol*>& out, std::string const& name) const {
      for (auto&& v : variables)
        if (v->name == name) out.push_back(v);

      return out.size();
    }

    size_t find_function(std::vector<Symbol*>& out, std::string const& name) const {
      for (auto&& v : functions)
        if (v->name == name) out.push_back(v);

      return out.size();
    }

    ModuleScope(NdModule* mod);
  };

  struct SymbolFindResult {

    std::vector<Symbol*> matches;

    std::vector<builtins::Function const*> blt_funcs;

    auto begin() { return matches.begin(); }
    auto end() { return matches.end(); }

    bool empty() const { return matches.empty(); }

    auto count() { return matches.size(); }

    Symbol* operator[](size_t i) { return matches[i]; }

    void remove(size_t at) { matches.erase(begin() + at); }

    inline void remove(std::vector<Symbol*>::iterator it) { matches.erase(it); }

    //
    // 絞り込み
    void narrow_down(std::vector<SymbolKind>&& kv) {
      for (auto it = begin(); it != end();) {
        for (auto& k : kv)
          if ((*it)->kind == k) {
            it++;
            goto __ok;
          }

        remove(it);
        continue;

      __ok:;
      }
    }
  };

  //
  // ExprTypeResult
  // 式の型を評価して、その型と他の情報を表す構造体
  struct ExprTypeResult {
    TypeInfo type;

    bool is_succeed = false;

    // 式中に含まれる要素の型に依存してる場合は true
    bool is_type_dependent = false;
    std::vector<Node*> depends;

    builtins::Function const* builtin_func;

    bool fail() const { return !is_succeed; }

    ExprTypeResult() {}

    ExprTypeResult(TypeInfo t) : type(std::move(t)) { is_succeed = true; }
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

    void check_let(NdLet* let);

    ExprTypeResult eval_expr(Node* node);

    ExprTypeResult eval_typename(NdSymbol* node);

  private:
    SymbolFindResult find_symbol(NdSymbol* node);

    static int get_required_template_params_count(Symbol* s);

    FunctionScope* get_cur_func_scope() {
      for (auto s = cur_scope; s; s = s->parent)
        if (s->is(NodeKind::Function)) return s->as<FunctionScope>();

      return nullptr;
    }
  };
} // namespace superman::sema