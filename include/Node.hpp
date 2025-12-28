#pragma once

#include <vector>

#include "Lexer.hpp"
#include "Object.hpp"
#include "Token.hpp"
#include "Utils.hpp"

namespace fire {

struct BuiltinFunc;

enum class NodeKind {
  Value,
  Symbol,
  KeyValuePair,
  Self,    // "self" keyword
  NullOpt, // "nullopt" keyword
  DeclType,
  Array,
  Tuple,
  Slice,
  Subscript,    // a[b]
  MemberAccess, // a.b

  CallFunc, // a(...)

  CF_CallBuiltin,
  CF_CallBuiltinMethod,
  CF_CallUserdefFn,
  CF_CallUserdefMethod,
  CF_MakeEnumerator,
  CF_MakeInstance,

  GetTupleElement, // a.0, a.1, ...
  Inclement,       // ++a or a++
  Declement,       // --a or a--
  BitNot,          // ~
  Not,             // !
  Ref,             // &a
  Deref,           // *a

  Mul,
  Div,
  Mod,
  Add,
  Sub,
  LShift,
  RShift,
  Bigger,        // a <  b
  BiggerOrEqual, // a <= b

  Equal,
  // NotEqual --> replace to !(a==b)

  BitAnd,
  BitXor,
  BitOr,
  LogAnd,
  LogOr,
  Assign,
  AssignWithOp, // +=, -=, ...

  Scope,
  Let,
  Try,
  Catch,
  If,
  Match,
  Switch,
  Loop,
  For,
  Do,
  While,

  Break,
  Continue,
  Return,

  FuncArgument,
  Function,

  Class,

  Enum,
  EnumeratorDef,

  Namespace,

  Module,
};

enum class NodePlaceholders {
  TemplateParam,
};

struct NdFunction;
struct NdClass;
//
// in Sema
struct Symbol;
struct VariableInfo;
struct Scope;

struct Node {
  NodeKind kind;
  Token token;
  std::string text;
  Scope* scope_ptr = nullptr;
  TypeInfo ty = {};
  bool ty_evaluated = false;
  template <typename T> T* as() { return static_cast<T*>(this); }
  bool is(NodeKind k) const { return kind == k; }
  bool is_expr() const {
    return kind >= NodeKind::Mul && kind <= NodeKind::AssignWithOp;
  }
  bool is_expr_full() const { return kind <= NodeKind::AssignWithOp; };
  virtual ~Node() {}

protected:
  Node(NodeKind k, Token const& t);
  Node(NodeKind kind, std::string const& text);
};

struct NdKeyValuePair : Node {
  Node* key;
  Node* value;
  NdKeyValuePair(Token const& t, Node* key, Node* value);
  ~NdKeyValuePair();
};

struct NdValue : Node {
  Object* obj = nullptr;

  NdValue(Token const& t);
  NdValue(Token const& t, Object* obj);
  ~NdValue();

  static NdValue* from_object(Object* obj, Token const& t) {
    auto v = new NdValue(t);
    v->obj = obj;
    return v;
  }
};

struct NdDeclType : Node {
  Node* expr;
  NdDeclType(Token const& tok, Node* expr = nullptr);
  ~NdDeclType();
};
//
// NdSymbol
//
struct NdSymbol : Node {
  Token name;
  NdDeclType* dec = nullptr;
  std::vector<NdSymbol*> te_args; // template-arguments
  Token* scope_resol_tok = nullptr;
  NdSymbol* next = nullptr; // scope-resolution
  Node* sym_target = nullptr;
  BuiltinFunc const* builtin_f = nullptr;
  bool is_ref = false;            //
  bool is_const = false;          //
  NdSymbol* concept_nd = nullptr; // if type-name
  bool is_local_var = false;
  bool is_global_var = false; //
  int var_offset = 0;         // if variable
  Symbol* symbol_ptr = nullptr;
  bool is_var() const { return is_local_var || is_global_var; }
  bool is_single() const { return !next; }

  NdSymbol(NdDeclType* de);
  NdSymbol(Token const& t, NdDeclType* de = nullptr);
  ~NdSymbol();

  static NdSymbol* from_str(std::string const& name) {
    return new NdSymbol(Token::from_str(name));
  }
};

struct NdOneToken : Node {
  NdOneToken(NodeKind kind, Token const& t);
  ~NdOneToken();
};

struct NdArray : Node {
  std::vector<Node*> data;
  NdArray(Token const& t);
  ~NdArray();
};

struct NdTuple : Node {
  std::vector<Node*> elems;
  NdTuple(Token const& t);
  ~NdTuple();
};
//
// NdCallFunc
//
struct NdCallFunc : Node {
  Node* callee;
  std::vector<Node*> args;
  bool is_method_call = false;
  Node* inst_expr = nullptr; // 'a' of "a.f()"
  Node* self_obj() { return inst_expr; }
  NdFunction* func_nd = nullptr;
  NdClass* class_nd = nullptr;
  BuiltinFunc const* builtin = nullptr;
  bool is_builtin() const { return !func_nd; }
  NdCallFunc(Node* callee, Token const& tok);
  ~NdCallFunc();
};

struct NdGetTupleElement : Node {
  Node* expr;
  int index;
  Token* index_tok = nullptr;
  NdGetTupleElement(Token const& tok, Node* expr, int index);
  ~NdGetTupleElement();
};

struct NdInclementDeclement : Node {
  Node* expr = nullptr;
  bool is_postfix = false; // true: ++a, false: a++
  NdInclementDeclement(NodeKind k, Token const& tok, Node* expr,
                       bool is_postfix);
  ~NdInclementDeclement();
};

struct NdOneExprWrap : Node {
  Node* expr = nullptr;
  NdOneExprWrap(NodeKind k, Token const& t, Node* e);
  ~NdOneExprWrap();
};

struct NdAssignWithOp : Node {
  NodeKind opkind;
  Node* lhs = nullptr;
  Node* rhs = nullptr;
  NdAssignWithOp(NodeKind opkind, Token& op, Node* l, Node* r);
  ~NdAssignWithOp();
};

struct NdExpr : Node {
  Node* lhs;
  Node* rhs;
  NdExpr(NodeKind k, Token& op, Node* l, Node* r);
  ~NdExpr();
};

struct NdLet : Node {
  Token name;

  NdSymbol* type = nullptr;
  Node* init = nullptr;

  bool is_pub = false; // when field

  // in Sema.
  int index = 0;
  Symbol* symbol_ptr = nullptr;

  std::vector<Token>
      placeholders; // when unpacking tuple or array or class-instance.

  NdLet(Token const& t, Token const& name);
  ~NdLet();
};

struct NdScope;
struct NdCatch : Node {
  Token holder;
  NdSymbol* error_type = nullptr;
  NdScope* body = nullptr;
  NdCatch(Token const& t);
  ~NdCatch();
};

struct NdTry : Node {
  NdScope* body = nullptr;
  std::vector<NdCatch*> catches;
  NdScope* finally_block = nullptr;
  NdTry(Token const& t);
  ~NdTry();
};

struct NdIf : Node {
  NdLet* vardef = nullptr;
  Node* cond = nullptr;
  NdScope* thencode = nullptr;
  Node* elsecode = nullptr;
  NdIf(Token const& t);
  ~NdIf();
};

struct NdFor : Node {
  Token iter;
  Node* iterable = nullptr;
  NdScope* body = nullptr;
  NdFor(Token const& t);
  ~NdFor();
};

struct NdWhile : Node {
  NdLet* vardef = nullptr;
  Node* cond = nullptr;
  NdScope* body = nullptr;
  NdWhile(Token const& t);
  ~NdWhile();
};

struct NdReturn : Node {
  Node* expr = nullptr;
  NdReturn(Token const& t);
  ~NdReturn();
};

struct NdBreakOrContinue : Node {
  NdBreakOrContinue(NodeKind k, Token& t);
  ~NdBreakOrContinue();
};

struct NdScope : Node {
  std::vector<Node*> items;
  NdScope(Token const& t);
  ~NdScope();
};

struct NdTemplatableBase : Node {
  std::vector<NdSymbol*> parameter_defs; // <T, U, ...>

  int count() const { return (int)parameter_defs.size(); }

  bool is_template() const { return count() != 0; }

protected:
  NdTemplatableBase(NodeKind k, Token const& t);
  ~NdTemplatableBase();
};

struct NdFuncArgument : Node {
  Token name;
  NdSymbol* type;
  VariableInfo* var_info_ptr = nullptr;
  NdFuncArgument(Token const& n, NdSymbol* type);
  ~NdFuncArgument();
};

struct NdFunction : NdTemplatableBase {
  Token name;
  std::vector<NdFuncArgument*> args;
  bool is_var_args = false;
  NdSymbol* result_type = nullptr;
  NdScope* body = nullptr;
  bool take_self = false; //
  bool is_pub = false;    // when method

  NdFunction(Token const& t, Token const& name);
  ~NdFunction();
};

struct NdEnumeratorDef;
struct NdEnum : Node {
  Token name;
  std::vector<NdEnumeratorDef*> enumerators;
  NdEnum(Token const& t);
  ~NdEnum();
};

struct NdEnumeratorDef : Node {
  enum VariantTypes {
    NoVariants,
    OneType,
    MultipleTypes,
    StructFields,
  };
  VariantTypes type = NoVariants;
  Token name;
  NdSymbol* variant = nullptr;
  std::vector<Node*> multiple;
  NdEnum* parent_enum_node = nullptr;
  std::string get_full_name() const {
    return parent_enum_node->name.text + "::" + name.text;
  }
  NdEnumeratorDef(Token const& t);
  ~NdEnumeratorDef();
};

struct NdClass : NdTemplatableBase {
  Token& name;
  NdSymbol* base_class = nullptr;
  std::vector<NdFunction*> methods;
  std::vector<NdLet*> fields;
  NdClass(Token const& tok, Token& name);
  ~NdClass();
};

struct NdNamespace : Node {
  std::string name;
  std::vector<Node*> items;
  NdNamespace(Token const& tok, std::string const& name);
  ~NdNamespace();
};

struct NdModule : Node {
  std::string name;
  std::vector<Node*> items;
  NdFunction* main_fn = nullptr;
  NdModule(Token const& tok);
  ~NdModule();
};

} // namespace fire