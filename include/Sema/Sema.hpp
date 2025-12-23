#pragma once

#include "Sema/Symbol.hpp"
#include "Sema/Scopes.hpp"

#define BIT(_N) (1 << (_N))

//
// スコープコンテキストを主軸として処理をまわす
//

namespace fire::sema {
  using namespace lexer;
  using namespace parser;

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

    vm::interp::BuiltinFunc const* builtin_f = nullptr;

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

    ExprType(Node* node, TypeInfo t) : node(node), type(std::move(t))
    {
      is_succeed = true;
    }
  };

  struct SymbolFindResult {

    std::vector<Symbol*> matches;

    vm::interp::BuiltinFunc const* builtin_f = nullptr;

    auto begin() { return matches.begin(); }
    auto end() { return matches.end(); }

    bool empty() const { return matches.empty(); }

    bool nothing()const{
      return empty() && !builtin_f;
    }

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
} // namespace superman::sema