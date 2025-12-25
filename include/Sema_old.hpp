#pragma once

#include <set>

#include "Node.hpp"

#define BIT(_N) (1 << (_N))

//
// スコープコンテキストを主軸として処理をまわす
//

namespace fire {
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

    bool is_type_name() const { return is(SymbolKind::Enum) || is(SymbolKind::Class) || is(SymbolKind::TemplateParam); }

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

    bool is_method = false;

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

  struct ClassScope : ScopeContext {
    SymbolTable fields;
    SymbolTable methods;

    Symbol* method_new = nullptr;
    Symbol* method_delete = nullptr;

    size_t find_symbol(std::vector<Symbol*>& out, std::string const& name) const override {
      return find_field(out, name) + find_method(out, name);
    }

    size_t find_field(std::vector<Symbol*>& out, std::string const& name) const {
      for (auto&& v : fields)
        if (v->name == name) out.push_back(v);

      return out.size();
    }

    size_t find_method(std::vector<Symbol*>& out, std::string const& name) const {
      for (auto&& v : methods)
        if (v->name == name) out.push_back(v);

      return out.size();
    }

    ClassScope(NdClass*);
  };

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

  //
  // ExprType
  // 式の型を評価して、その型と他の情報を表す構造体
  struct ExprType {

    Node* node = nullptr;

    TypeInfo type;

    bool is_succeed = false;

    // 式中に含まれる要素の型に依存してる場合は true
    bool is_type_dependent = false;
    std::vector<Node*> depends;

    BuiltinFunc const* builtin_f = nullptr;

    //
    //  func_nd
    //  - set pointer if node is:
    //    - NdSymbol
    //    - NdCallFunc
    NdFunction* func_nd = nullptr;

    //
    //  class_nd
    //  - set pointer if node is:
    //    - NdSymbol
    //    - NdNew
    NdClass* class_nd = nullptr;

    bool fail() const { return !is_succeed; }

    ExprType(Node* node = nullptr) : node(node) {}

    ExprType(Node* node, TypeInfo t) : node(node), type(std::move(t)) { is_succeed = true; }
  };

  struct SymbolFindResult {

    std::vector<Symbol*> matches;

    BuiltinFunc const* builtin_f = nullptr;

    auto begin() { return matches.begin(); }
    auto end() { return matches.end(); }

    bool empty() const { return matches.empty(); }

    bool nothing() const { return empty() && !builtin_f; }

    size_t count() { return matches.size(); }

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

  class Sema {
    ModuleScope* root_scope = nullptr;

    ScopeContext* cur_scope = nullptr;

  public:
    Sema(NdModule* mod);

    static void analyze_all(NdModule* mod);

    void analyze_full();

    void check_module(ModuleScope* mod);
    void check_class(ClassScope* func);
    void check_function(FunctionScope* func);
    void check_scope(UnnamedScope* scope);

    void check_let(NdLet* let);

    ExprType eval_expr(Node* node);

    ExprType eval_typename(NdSymbol* node);

    TypeInfo make_class_type(NdClass*);

  private:
    SymbolFindResult find_symbol(NdSymbol* node);

    static int get_required_template_params_count(Symbol* s);

    FunctionScope* get_cur_func_scope() {
      for (auto s = cur_scope; s; s = s->parent)
        if (s->is(NodeKind::Function)) return s->as<FunctionScope>();

      return nullptr;
    }
  };
} // namespace fire