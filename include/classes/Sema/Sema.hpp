#pragma once

#include "Utils.hpp"
#include "Node.hpp"

#include "Sema/Symbols.hpp"
#include "Sema/Scope.hpp"

/*

1. シンボル定義情報を収集

2. 名前の参照の解決

3. 型チェック
   - 型の一致確認
   - 型推論

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

    Scope* cur_scope = nullptr;

    SCFunction* cur_func = nullptr;

    TypeInfo* expected_type = nullptr;
  };

  struct SymbolFindResult {
    NdSymbol* node = nullptr;
    NdSymbol* previous = nullptr; // "a" of "a::b"
    std::vector<Symbol*> hits = {};
  };

  class Sema {

    SCModule* root_scope = nullptr;

  public:
    Sema();

    static void analyze_all(NdModule* mod);

    void analyze_full(NdModule* mod);

    Scope* create_scope(Node* node, Scope* parent);

    NdVisitorContext resolve_names(Node* node, NdVisitorContext ctx);

    void infer_types(Node* node);

    void check_semantics(Node* node);

    Symbol* new_variable_symbol(NdLet* let);

    Symbol* new_variable_symbol(Token* tok, std::string const& name);

  private:
    TypeInfo eval_expr_ty(Node* node, NdVisitorContext ctx);

    TypeInfo eval_typename_ty(NdSymbol* node, NdVisitorContext ctx);

    SymbolFindResult find_symbol(NdSymbol* node, NdVisitorContext ctx);
  };

} // namespace fire