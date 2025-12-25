#pragma once

#include <vector>

#include "Lexer.hpp"
#include "Token.hpp"
#include "Object.hpp"

namespace fire {

  struct BuiltinFunc;

  enum class NodeKind {
    Value,

    Symbol,

    KeyValuePair,

    Self, // "self" keyword

    DeclType,

    Array,
    Tuple,

    Slice,
    Subscript, // a[b]

    MemberAccess, // a.b
    CallFunc,     // a(...)

    Inclement, // ++a or a++
    Declement, // --a or a--

    BitNot, // ~
    Not,    // !
    Plus,   // +a
    Minus,  // -a

    Ref,   // &a
    Deref, // *a

    New,    //
    Delete, //

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

    // stmt ::= scope | if | for | break | continue | return
    //          | (expr ";")

    // scope ::= "{" stmt* "}"
    Scope,

    Let,

    // if  ::=  if expr stmt ("else" stmt)?
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

    Namespace,

    Module,
  };

  struct NdFunction;

  //
  // Sema.hpp
  struct ScopeContext;
  struct VariableInfo;

  struct Node {
    NodeKind kind;

    Token token;
    std::string text;

    ScopeContext* scope_ptr = nullptr;

    template <typename T>
    T* as() {
      return static_cast<T*>(this);
    }

    bool is(NodeKind k) const { return kind == k; }

    bool is_expr() const { return kind >= NodeKind::Mul && kind <= NodeKind::Assign; }

    bool is_expr_full() const { return kind <= NodeKind::Assign; };

    virtual ~Node() {}

  protected:
    Node(NodeKind k, Token& t) : kind(k), token(t), text(t.text) {}
    Node(NodeKind kind, std::string const& text) : kind(kind), text(text) {}
  };

  struct NdKeyValuePair : Node {
    Node* key;
    Node* value;
    NdKeyValuePair(Token& t, Node* key, Node* value) : Node(NodeKind::KeyValuePair, t), key(key), value(value) {}
  };

  struct NdValue : Node {
    Object* obj = nullptr;
    NdValue(Token& t) : Node(NodeKind::Value, t) {}
  };

  struct NdDeclType : Node {
    Node* expr;
    NdDeclType(Token& tok, Node* expr = nullptr) : Node(NodeKind::DeclType, tok), expr(expr) {}
  };

  //
  // NdSymbol
  //
  struct NdSymbol : Node {
    Token name;

    NdDeclType* dec = nullptr;

    std::vector<NdSymbol*> te_args; // template-arguments
    NdSymbol* next = nullptr;       // scope-resolution

    Node* sym_target = nullptr;

    BuiltinFunc const* builtin_f = nullptr;

    bool is_ref = false;            //
    bool is_const = false;          //
    NdSymbol* concept_nd = nullptr; // if type-name

    bool is_local_var = false;
    bool is_global_var = false; //
    int var_offset = 0;         // if variable

    bool is_var() const { return is_local_var || is_global_var; }

    NdSymbol(NdDeclType* de) : Node(NodeKind::Symbol, de->token), name(de->token), dec(de) {}

    NdSymbol(Token& t) : Node(NodeKind::Symbol, t), name(token) {}

    bool is_single() const { return !next; }
  };

  struct NdSelf : Node {
    NdSelf(Token& t) : Node(NodeKind::Self, t) {}
  };

  struct NdArray : Node {
    std::vector<Node*> data;
    NdArray(Token& t) : Node(NodeKind::Array, t) {}
  };

  struct NdTuple : Node {
    std::vector<Node*> elems;
    NdTuple(Token& t) : Node(NodeKind::Tuple, t) {}
  };

  //
  // NdCallFunc
  //
  struct NdCallFunc : Node {
    Node* callee;
    std::vector<Node*> args;

    bool is_method_call = false;
    Node* inst_expr = nullptr; // 'a' of "a.f()"

    // inst_expr はあるが、args にも同じものを先頭に追加します

    NdFunction* func_nd = nullptr;
    BuiltinFunc const* builtin = nullptr;

    bool is_builtin() const { return !func_nd; }

    NdCallFunc(Node* callee, Token& tok) : Node(NodeKind::CallFunc, tok), callee(callee) {}
  };

  struct NdInclement : Node {
    Node* expr = nullptr;
    bool is_postfix = false; // true: ++a, false: a++
    NdInclement(Token& tok, Node* expr, bool is_postfix)
        : Node(NodeKind::Inclement, tok), expr(expr), is_postfix(is_postfix) {}
  };

  struct NdDeclement : Node {
    Node* expr = nullptr;
    bool is_postfix = false; // true: --a, false: a--
    NdDeclement(Token& tok, Node* expr, bool is_postfix)
        : Node(NodeKind::Declement, tok), expr(expr), is_postfix(is_postfix) {}
  };

  struct NdNew : Node {
    NdSymbol* type;
    std::vector<Node*> args;
    NdNew(Token& tok) : Node(NodeKind::New, tok) {}
  };

  struct NdRef : Node {
    Node* expr = nullptr;
    NdRef(Token& tok) : Node(NodeKind::Ref, tok) {}
  };

  struct NdDeref : Node {
    Node* expr = nullptr;
    NdDeref(Token& tok) : Node(NodeKind::Deref, tok) {}
  };

  // !a
  struct NdNot : Node {
    Node* expr = nullptr;
    NdNot(Token& tok) : Node(NodeKind::Not, tok) {}
  };

  // ~a
  struct NdBitNot : Node {
    Node* expr = nullptr;
    NdBitNot(Token& tok) : Node(NodeKind::BitNot, tok) {}
  };

  struct NdExpr : Node {
    Node* lhs;
    Node* rhs;
    NdExpr(NodeKind k, Token& op, Node* l, Node* r) : Node(k, op), lhs(l), rhs(r) {}
  };

  struct NdLet : Node {
    Token& name;

    bool is_static = false;

    std::vector<Token*> placeholders; // when unpacking tuple.

    NdSymbol* type = nullptr;
    Node* init = nullptr;

    bool is_pub = false; // when field

    int index = 0;

    VariableInfo* var_info_ptr = nullptr;

    NdLet(Token& t, Token& name) : Node(NodeKind::Let, t), name(name) {}
  };

  struct NdScope;

  struct NdIf : Node {
    NdLet* vardef = nullptr;
    Node* cond = nullptr;
    Node* thencode = nullptr;
    Node* elsecode = nullptr;
    NdIf(Token& t) : Node(NodeKind::If, t) {}
  };

  struct NdFor : Node {
    Token iter;
    Node* iterable = nullptr;
    NdScope* body = nullptr;
    NdFor(Token& t) : Node(NodeKind::For, t) {}
  };

  struct NdWhile : Node {
    NdLet* vardef = nullptr;
    Node* cond = nullptr;
    NdScope* body = nullptr;
    NdWhile(Token& t) : Node(NodeKind::While, t) {}
  };

  struct NdReturn : Node {
    Node* expr = nullptr;
    NdReturn(Token& t) : Node(NodeKind::Return, t) {}
  };

  struct NdBreakOrContinue : Node {
    NdBreakOrContinue(NodeKind k, Token& t) : Node(k, t) {}
  };

  struct NdScope : Node {
    std::vector<Node*> items;
    NdScope(Token& t) : Node(NodeKind::Scope, t) {}
  };

  struct NdTemplatableBase : Node {
    std::vector<NdSymbol*> parameter_defs; // <T, U, ...>

    int count() const { return (int)parameter_defs.size(); }

    bool is_template() const { return count() != 0; }

  protected:
    NdTemplatableBase(NodeKind k, Token& t) : Node(k, t) {}
  };

  struct NdFunction : NdTemplatableBase {
    struct Argument : Node {
      Token& name;
      NdSymbol* type;

      VariableInfo* var_info_ptr = nullptr;

      Argument(Token& n, NdSymbol* type) : Node(NodeKind::FuncArgument, n), name(n), type(type) {}
    };

    Token& name;
    std::vector<Argument> args;
    NdSymbol* result_type = nullptr;
    NdScope* body = nullptr;

    bool take_self = false; //
    bool is_pub = false;    // when method

    NdFunction(Token& t, Token& name) : NdTemplatableBase(NodeKind::Function, t), name(name) {}
  };

  struct NdEnum : Node {
    NdEnum(Token& t) : Node(NodeKind::Enum, t) {}
  };

  struct NdClass : NdTemplatableBase {
    Token& name;
    NdSymbol* base_class = nullptr;
    std::vector<NdFunction*> methods;
    std::vector<NdLet*> fields;

    NdFunction* m_new = nullptr;
    NdFunction* m_delete = nullptr;

    NdClass(Token& tok, Token& name) : NdTemplatableBase(NodeKind::Class, tok), name(name) {}
  };

  struct NdNamespace : Node {
    std::string name;
    std::vector<Node*> items;

    NdNamespace(Token& tok, std::string const& name) : Node(NodeKind::Namespace, tok), name(name) {}
  };

  struct NdModule : Node {
    std::string name;
    std::vector<Node*> items;

    NdFunction* main_fn = nullptr;

    NdModule(Token& tok) : Node(NodeKind::Module, tok) {}
  };
} // namespace fire