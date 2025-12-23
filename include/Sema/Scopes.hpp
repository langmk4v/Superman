#pragma once

#include "Sema/Symbol.hpp"

//
// スコープコンテキストを主軸として処理をまわす
//

namespace fire::sema {
  using namespace lexer;
  using namespace parser;

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
}