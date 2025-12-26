#pragma once

#include "Utils.hpp"
#include "Node.hpp"

#include "Sema/Symbols.hpp"
#include "Sema/Scope.hpp"

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

  struct VariableInfo {
    TypeInfo type = {};
    bool is_type_deducted = false;

    int offset = 0;
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

    // expression-types
    bool as_typename = false;
    bool as_callee_of_callfunc = false;

    bool as_arg_of_callfunc = false;
    NdCallFunc* parent_cf_nd = nullptr;
    TypeInfo* self_ty_ptr = nullptr;

    NdEnumeratorDef** enumerator_node_out = nullptr;
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

    void infer_types(Node* node);

    void check_semantics(Node* node);

    Symbol* new_variable_symbol(NdLet* let);

    Symbol* new_variable_symbol(Token* tok, std::string_view name);

  private:
    Sema();

    SymbolFindResult find_symbol(NdSymbol* node, NdVisitorContext ctx);
  };

} // namespace fire