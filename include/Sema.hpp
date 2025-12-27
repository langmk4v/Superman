#pragma once

#include "Utils.hpp"
#include "Node.hpp"

/*

1. 名前解決
   - わかる範囲のみ・型評価はしない・エラーは出さない

2. 型チェック
   - 型の一致確認
   - 型推論
   - (1) で名前解決できなかった部分を解決する

4. 文をチェック
   - return, break, continue が使えるかどうか

5. 式の意味チェック
   - 型と演算子が一致するかどうか
   - 型に対して演算子が使用可能か

6. 制御フローの確認

*/

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
    TemplateParam,
    BuiltinType,
    BuiltinFunc,
  };

  struct VariableInfo;
  struct Scope;

  struct BuiltinFunc;

  struct Symbol {
    std::string name = "";
    SymbolKind kind = SymbolKind::Unknown;
    TypeInfo type = {};
    Node* node = nullptr;
    Token* token = nullptr;
    VariableInfo* var_info = nullptr;
    BuiltinFunc const* builtin_f = nullptr;
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

  enum class ScopeKind {
    Unknown,

    Scope,

    If,
    For,
    While,
    Try,
    Catch,

    Func,
    Enum,
    Class,

    Namespace,
    Module,
  };

  struct Scope {
    ScopeKind kind = ScopeKind::Unknown;

    Node* node = nullptr;
    Scope* parent = nullptr;

    Symbol symbol;
    SymbolTable symtable;

    template <typename T>
    T* as() {
      return static_cast<T*>(this);
    }

    virtual ~Scope() = default;

    Scope* from_node(Node* node, Scope* parent);

  protected:
    Scope(ScopeKind kind, Node* node, Scope* parent);
  };

  struct SCScope : Scope {
    SymbolTable variables;
    std::vector<Scope*> subscopes;

    SCScope(NdScope* node, Scope* parent);
  };

  struct SCIf : Scope {
    Symbol* var = nullptr;
    SCScope* then_scope = nullptr;
    SCScope* else_scope = nullptr;

    SCIf(NdIf* node, Scope* parent);
  };

  struct SCFor : Scope {
    Symbol* iter_name = nullptr;
    SCScope* body = nullptr;

    SCFor(NdFor* node, Scope* parent);
  };

  struct SCWhile : Scope {
    Symbol* var = nullptr;
    SCScope* body = nullptr;

    SCWhile(NdWhile* node, Scope* parent);
  };

  struct SCCatch : Scope {
    Symbol* holder_name = nullptr;
    SCScope* body = nullptr;

    SCCatch(NdCatch* node, Scope* parent);
  };

  struct SCTry : Scope {
    SCScope* body = nullptr;
    std::vector<SCCatch*> catches;
    SCScope* finally_scope = nullptr;

    SCTry(NdTry* node, Scope* parent);
  };

  struct SCFunction : Scope {
    SymbolTable arguments;
    SCScope* body = nullptr;
    
    size_t var_max_count = 0;

    SCFunction(NdFunction* node, Scope* parent);
  };

  struct SCEnum : Scope {
    SymbolTable enumerators;
    SCEnum(NdEnum* node, Scope* parent);
  };

  struct SCClass : Scope {
    SymbolTable fields;
    SymbolTable methods;

    NdClass* get_node() {
      return node->as<NdClass>();
    }

    SCClass(NdClass* node, Scope* parent);
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

    SCNamespace(NdNamespace* node, Scope* parent);
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

    SCModule(NdModule* node, Scope* parent);
  };

  struct VariableInfo {
    TypeInfo type = {};
    bool is_type_deducted = false;

    size_t offset = 0;
  };

  struct NdVisitorContext {
    Node* node = nullptr;

    //
    // Location infos
    Scope* cur_scope = nullptr;
    SCClass* cur_class = nullptr;
    SCFunction* cur_func = nullptr;
    bool in_method = false;

    //
    // element-type of empty-array
    TypeInfo* empty_array_element_type = nullptr;
    bool can_use_empty_array = false;

    //
    // slice-target-array-type
    TypeInfo* slice_target_array_type = nullptr;

    int loop_depth = 0;

    TypeInfo* expected_type = nullptr;
    TypeInfo* expected_return_type = nullptr;

    // expression-types
    bool as_typename = false;
    bool as_callee_of_callfunc = false;

    bool as_arg_of_callfunc = false;
    NdCallFunc* parent_cf_nd = nullptr;
    TypeInfo* self_ty_ptr = nullptr;

    NdEnumeratorDef** enumerator_node_out = nullptr;

  #ifdef _FIRE_DEBUG_
    static std::string ctx2s(NdVisitorContext ctx) {
      std::string flags;
      flags+=format("node: %p", ctx.node);
      if(ctx.cur_scope) flags+=format("cur_scope: %p,", ctx.cur_scope);
      if(ctx.cur_class) flags+=format("cur_class: %p,", ctx.cur_class);
      if(ctx.cur_func) flags+=format("cur_func: %p,", ctx.cur_func);
      if(ctx.in_method) flags+=format("in_method: %d", ctx.in_method);
      if(ctx.empty_array_element_type) flags+=format("empty_array_element_type: %p,", ctx.empty_array_element_type);
      if(ctx.can_use_empty_array) flags+=format("can_use_empty_array: %d,", ctx.can_use_empty_array);
      if(ctx.slice_target_array_type) flags+=format("slice_target_array_type: %p,", ctx.slice_target_array_type);
      if(ctx.loop_depth) flags+=format("loop_depth: %d,", ctx.loop_depth);
      if(ctx.expected_type) flags+=format("expected_type: %p,", ctx.expected_type);
      if(ctx.as_typename) flags+=format("as_typename: %d,", ctx.as_typename);
      if(ctx.as_callee_of_callfunc) flags+=format("as_callee_of_callfunc: %d,", ctx.as_callee_of_callfunc);
      if(ctx.as_arg_of_callfunc) flags+=format("as_arg_of_callfunc: %d,", ctx.as_arg_of_callfunc);
      if(ctx.parent_cf_nd) flags+=format("parent_cf_nd: %p,", ctx.parent_cf_nd);
      if(ctx.self_ty_ptr) flags+=format("self_ty_ptr: %p,", ctx.self_ty_ptr);
      if(ctx.enumerator_node_out) flags+=format("enumerator_node_out: %p,", ctx.enumerator_node_out);
      return "{"+flags+"}";
    }
  #endif

  };

  class Sema;

  class NameResolver {
    friend class Sema;

    Sema& S;

  public:
    NameResolver(Sema& S) : S(S) {
    }

    void on_typename(Node* node, NdVisitorContext ctx);

    void on_expr(Node* node, NdVisitorContext ctx);

    void on_stmt(Node* node, NdVisitorContext ctx);
    void on_scope(Node* node, NdVisitorContext ctx);

    void on_function(Node* node, NdVisitorContext ctx);
    void on_class(Node* node, NdVisitorContext ctx);
    void on_enum(Node* node, NdVisitorContext ctx);
    void on_namespace(Node* node, NdVisitorContext ctx);
    void on_module(Node* node, NdVisitorContext ctx);
  };

  class TypeChecker {
    friend class Sema;

    Sema& S;

  public:
    TypeChecker(Sema& S) : S(S) {
    }

    struct ArgumentsCompareResult {
      enum Flags {
        Default = 0,

        // no match counts.
        TooMany = BIT(1),
        TooFew = BIT(2),

        // type mismatch.
        TypeMismatch = BIT(3),
      };

      int flags = static_cast<int>(Default);
      size_t mismatched_index = 0;
    };

    ArgumentsCompareResult compare_arguments(
        NdCallFunc* cf, NdFunction* fn, BuiltinFunc const* builtin,
        bool is_var_arg, bool is_method_call,
        TypeInfo& self_ty, std::vector<TypeInfo> const& defs, std::vector<TypeInfo>& actual);

    TypeInfo case_call_func(NdCallFunc* cf, NdVisitorContext ctx);
    TypeInfo case_method_call(NdCallFunc* cf, NdVisitorContext ctx);

    TypeInfo case_construct_enumerator(
      NdCallFunc* cf, NdEnumeratorDef* en_def, TypeInfo& callee_ty,
      size_t argc_give, std::vector<TypeInfo>& arg_types, NdVisitorContext ctx);

    TypeInfo eval_expr_ty(Node* node, NdVisitorContext ctx);
    TypeInfo eval_typename_ty(NdSymbol* node, NdVisitorContext ctx);

    TypeInfo make_class_type(NdClass* node);

    TypeInfo make_enum_type(NdEnum* node);

    void check_expr(Node* node, NdVisitorContext ctx);
    void check_stmt(Node* node, NdVisitorContext ctx);
    void check_scope(NdScope* node, NdVisitorContext ctx);
    void check_function(NdFunction* node, NdVisitorContext ctx);
    void check_class(NdClass* node, NdVisitorContext ctx);
    void check_enum(NdEnum* node, NdVisitorContext ctx);
    void check_namespace(NdNamespace* node, NdVisitorContext ctx);
    void check_module(NdModule* node, NdVisitorContext ctx);
  };

  struct SymbolFindResult {
    NdSymbol* node = nullptr;
    NdSymbol* previous = nullptr; // "a" of "a::b"
    std::vector<Symbol*> hits = {};
  };

  class Sema {

    friend class NameResolver;

    SCModule* root_scope = nullptr;

  public:
    static Sema& get_instance();

    static void analyze_all(NdModule* mod);

    void analyze_full(NdModule* mod);

    Symbol* new_variable_symbol(NdLet* let);

    Symbol* new_variable_symbol(Token* tok, std::string_view name);

  private:
    Sema();

    SymbolFindResult find_symbol(NdSymbol* node, NdVisitorContext ctx);
  };

} // namespace fire