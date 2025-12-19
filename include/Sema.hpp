#pragma once

#include "Node.hpp"
#include "TypeInfo.hpp"

/*
・シンボルテーブルを主軸として処理する
・シンボルに情報を持たせる
*/

namespace superman::sema {
  enum class SymbolKind {
    Var,
    Func,
    Type,
    Module,
  };

  struct _symbol_context_base;
  struct FunctionContext;
  struct EnumContext;
  struct ClassContext;
  struct ModuleContext;
  struct Symbol {
    SymbolKind kind;
    std::string name;

    union {
      _symbol_context_base* ctx = nullptr;

      FunctionContext* c_func;
      EnumContext* c_enum;
      ClassContext* c_class;
      ModuleContext* c_mod;
    };

    Symbol(SymbolKind k, std::string const& n) : kind(k), name(n) {}
  };

  struct SymbolTable {
    SymbolTable* parent;
    std::vector<Symbol*> data;

    Symbol*& append(Symbol* s) { return data.emplace_back(s); }

    SymbolTable(SymbolTable* parent = nullptr) : parent(parent), data() {}
  };

  enum SymbolContextType {
    SC_Func,
    SC_Enum,
    SC_Class,
    SC_Module,
  };

  struct _symbol_context_base {
    Symbol* self;
    SymbolContextType type;
    _symbol_context_base(Symbol* S, SymbolContextType t) : self(S), type(t) {}
  };

  struct FunctionContext : _symbol_context_base {
    SymbolTable variables;

    static Symbol* make_symbol(NdFunction* func) {
      auto sym = new Symbol(SymbolKind::Func, func->name.text);
      sym->ctx = new FunctionContext(sym);
      return sym;
    }

  private:
    FunctionContext(Symbol* self) : _symbol_context_base(self, SC_Func) {}
  };

  struct EnumContext : _symbol_context_base {
    SymbolTable enumerators;

    EnumContext(Symbol* s) : _symbol_context_base(s, SC_Enum) {}
  };

  struct ModuleContext : _symbol_context_base {
    SymbolTable mod_items;

    static Symbol* make_symbol(NdModule* mod) {
      auto sym = new Symbol(SymbolKind::Module, mod->name);
      sym->ctx = new ModuleContext(sym);

      for (auto item : mod->items) {
        switch (item->kind) {
        case NodeKind::Function:
          sym->c_mod->mod_items.append(FunctionContext::make_symbol(item->as<NdFunction>()));
          break;

        case NodeKind::Module:
          sym->c_mod->mod_items.append(ModuleContext::make_symbol(item->as<NdModule>()));
          break;
        }
      }

      return sym;
    }

  private:
    ModuleContext(Symbol* self) : _symbol_context_base(self, SC_Module) {}
  };

  //
  // structure for type evaluation result of an expression
  struct ExprTypeResult {
    TypeInfo type;

    bool is_succeed = false;

    bool is_type_dependent = false;

    std::vector<Node*> depends;

    bool fail() const { return !is_succeed; }

    ExprTypeResult() {}

    ExprTypeResult(TypeInfo t) : type(std::move(t)) {}
  };

  struct SymbolFindResult {

    std::vector<Symbol*> v;

    bool nothing() const { return v.empty(); }

    auto count() { return v.size(); }

    Symbol* operator[](size_t i) { return v[i]; }

    void remove(size_t at) { v.erase(v.begin() + at); }
  };

  class Sema {
    Symbol* mod_sym;

    SymbolTable* cur;

  public:
    Sema(NdModule* mod) {
      mod_sym = ModuleContext::make_symbol(mod);

      cur = &mod_sym->c_mod->mod_items;

      alert;
    }

    void check_full() { check_module(mod_sym->c_mod); }

    void check_function(NdFunction* node) { (void)node; }

    void check_module(ModuleContext* modctx) {}

    ExprTypeResult eval_expr(Node* node) {
      switch (node->kind) {
      case NodeKind::Value:
        return node->as<NdValue>()->obj->type;
      }

      todoimpl;
    }

    ExprTypeResult eval_typename(NdSymbol* node) {
      std::pair<char const*, TypeKind> name_kind_pairs[]{
          {"none", TypeKind::None},     {"int", TypeKind::Int},
          {"float", TypeKind::Float},   {"bool", TypeKind::Bool},
          {"char", TypeKind::Char},     {"string", TypeKind::String},
          {"vector", TypeKind::Vector}, {"tuple", TypeKind::Tuple},
          {"dict", TypeKind::Dict},     {"function", TypeKind::Function},
      };

      auto result = ExprTypeResult();

      // 基本型の名前から探す
      for (auto&& [s, k] : name_kind_pairs)
        if (node->name.text == s) {
          result.type.kind = k;

          // テンプレート引数が多すぎるまたは少なすぎる
          if (int C = TypeInfo::required_param_count(k), N = (int)node->te_args.size(); C != N)
            throw err::no_match_template_arguments(node->name, C, N);

          // 存在したらユーザー定義型の検索はスキップ
          goto _Skip_Find_Userdef;
        }

      {
        auto found = find_symbol(node);

        if (found.nothing()) throw err::unknown_type_name(node->name);

        for (size_t i = 0; i < found.count();) {
          if (found[i]->kind != SymbolKind::Type) {
            found.remove(i);
            continue;
          }
          i++;
        }

        if (found.count() >= 2) {
          throw err::ambiguous_symbol_name(node->name);
        }

        auto Final = found[0];
      }
    _Skip_Find_Userdef:;

      return result;
    }

    SymbolFindResult find_symbol(NdSymbol* node) {
      auto r = SymbolFindResult();

      return r;
    }

    static int get_required_template_params_count(Symbol* s) {
      if (s->kind == SymbolKind::Type) {
        if (s->ctx->type == SC_Enum) {
          todoimpl;
        }
        if (s->ctx->type == SC_Class) {
          todoimpl;
        }
      }

      if (s->kind == SymbolKind::Func) {
        todoimpl;
      }

      if (s->kind == SymbolKind::Var) {
        // variable is still not template yet... ?
        todoimpl;
      }

      return 0; // no needed.
    }
  };
} // namespace superman::sema