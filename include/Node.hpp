#pragma once

#include <vector>
#include "Token.hpp"

namespace superman {

  struct Token;
  struct Object;

  enum class NodeKind {
    Value,
    Symbol,

    Array,
    Tuple,

    Subscript,    // a[b]
    MemberAccess, // a.b
    CallFunc,     // a(...)

    PreInclement, // ++a
    PreDeclement, // --a
    BitNot,       // ~
    Not,          // !
    Plus,         // +a
    Minus,        // -a

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

    For,
    Break,
    Continue,
    Return,

    Function,

    Class,

    Enum,

    Module,
  };

  struct Node {
    NodeKind kind;
    Token& token;

    template <typename T>
    T* as() {
      return static_cast<T*>(this);
    }

    virtual ~Node() {}

  protected:
    Node(NodeKind k, Token& t) : kind(k), token(t) {}
  };

  struct NdValue : Node {
    Object* obj = nullptr;
    NdValue(Token& t) : Node(NodeKind::Value, t) {}
  };

  struct NdSymbol : Node {
    enum Types {
      Unknown,
      Var,
      Func,
      Type,
    };

    Types type = Unknown;
    Token& name;
    std::vector<NdSymbol*> te_args; // template-arguments
    NdSymbol* next = nullptr;       // scope-resolution

    Node* sym_target = nullptr; // <-- sema

    bool is_ref = false;            //
    bool is_const = false;          //
    NdSymbol* concept_nd = nullptr; // if type-name

    NdSymbol(Token& t) : Node(NodeKind::Symbol, t), name(token) {}

    bool is_single() const { return !next; }
  };

  struct NdNew : Node {
    NdSymbol* type;
    std::vector<Node*> args;
    NdNew(Token& tok) : Node(NodeKind::New, tok) {}
  };

  struct NdArray : Node {
    std::vector<Node*> data;
    NdArray(Token& t) : Node(NodeKind::Array, t) {}
  };

  struct NdTuple : Node {
    std::vector<Node*> elems;
    NdTuple(Token& t) : Node(NodeKind::Tuple, t) {}
  };

  struct NdCallFunc : Node {
    Node* callee;
    std::vector<Node*> args;
    NdCallFunc(Node* callee, Token& tok) : Node(NodeKind::CallFunc, tok), callee(callee) {}
  };

  struct NdExpr : Node {
    Node* lhs;
    Node* rhs;
    NdExpr(NodeKind k, Token& op, Node* l, Node* r) : Node(k, op), lhs(l), rhs(r) {}
  };

  struct NdLet : Node {
    Token& name;
    NdSymbol* type;
    Node* init;

    bool is_pub = false; // when field

    NdLet(Token& t, Token& name) : Node(NodeKind::Let, t), name(name) {}
  };

  struct NdIf : Node {
    Node* cond;
    Node* thencode;
    Node* elsecode;
    NdIf(Token& t) : Node(NodeKind::If, t) {}
  };

  struct NdReturn : Node {
    Node* expr;
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
    struct Argument {
      Token& name;
      NdSymbol* type;
      Argument(Token& n, NdSymbol* type) : name(n), type(type) {}
    };
    Token& name;
    std::vector<Argument> args;
    NdSymbol* result_type = nullptr;
    NdScope* body = nullptr;

    bool take_self = false; //
    bool is_pub = false;    // when method

    NdFunction(Token& t, Token& name) : NdTemplatableBase(NodeKind::Function, t), name(name) {}
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

  struct NdModule : Node {
    std::string name;
    std::vector<Node*> items;
    NdModule(Token& tok) : Node(NodeKind::Module, tok) {}
  };
} // namespace superman