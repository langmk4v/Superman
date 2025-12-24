#pragma once

#include <vector>

#include "Lexer/Token.hpp"
#include "VM/Interp/Builtins.hpp"
#include "Sema/fwd.hpp"

namespace fire {
  namespace lexer {
    struct SourceCode;
  }

  namespace vm::interp {
    struct Object;
  }
}

namespace fire::parser {

  using lexer::Token;
  using lexer::SourceCode;

  using vm::interp::Object;

  enum class NodeKind {
    Value,

    Symbol,

    Self, // "self" keyword

    DeclType,

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

    FuncArgument,
    Function,

    Class,

    Enum,

    Module,
  };

  struct NdFunction;

  struct Node {
    NodeKind kind;
    Token& token;

    sema::ScopeContext* scope_ptr = nullptr;

    template <typename T>
    T* as() {
      return static_cast<T*>(this);
    }

    bool is(NodeKind k) const { return kind == k; }

    bool is_expr() const { return kind >= NodeKind::Mul && kind <= NodeKind::Assign; }

    bool is_expr_full() const { return kind <= NodeKind::Assign; };

    virtual ~Node() {}

  protected:
    Node(NodeKind k, Token& t) : kind(k), token(t) {}
  };

  struct NdValue : Node {
    Object* obj = nullptr;
    NdValue(Token& t) : Node(NodeKind::Value, t) {}
  };

  struct NdDeclType : Node {
    Node* expr;
    NdDeclType(Token& tok, Node* expr = nullptr) : Node(NodeKind::DeclType, tok), expr(expr) { }
  };

  //
  // NdSymbol
  //
  struct NdSymbol : Node {
    Token& name;

    NdDeclType* dec=nullptr;

    std::vector<NdSymbol*> te_args; // template-arguments
    NdSymbol* next = nullptr;       // scope-resolution

    Node* sym_target = nullptr;
    
    vm::interp::BuiltinFunc const* builtin_f = nullptr;

    bool is_ref = false;            //
    bool is_const = false;          //
    NdSymbol* concept_nd = nullptr; // if type-name

    bool is_local_var=false;
    bool is_global_var = false; //
    int var_offset = 0;         // if variable

    bool is_var()const{return is_local_var || is_global_var;}

    NdSymbol(NdDeclType* de) : Node(NodeKind::Symbol, de->token), name(de->token), dec(de) { }

    NdSymbol(Token& t) : Node(NodeKind::Symbol, t), name(token) {}

    bool is_single() const { return !next; }
  };

  struct NdSelf : Node {
    NdSelf(Token&t):Node(NodeKind::Self,t){}
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

  //
  // NdCallFunc
  //
  struct NdCallFunc : Node {
    Node* callee;
    std::vector<Node*> args;

    bool is_method_call = false;
    Node* inst_expr =nullptr; // 'a' of "a.f()"

    // inst_expr はあるが、args にも同じものを先頭に追加します

    NdFunction* func_nd = nullptr;
    vm::interp::BuiltinFunc const* builtin = nullptr;

    bool is_builtin() const { return !func_nd; }

    NdCallFunc(Node* callee, Token& tok) : Node(NodeKind::CallFunc, tok), callee(callee) {}
  };

  struct NdExpr : Node {
    Node* lhs;
    Node* rhs;
    NdExpr(NodeKind k, Token& op, Node* l, Node* r) : Node(k, op), lhs(l), rhs(r) {}
  };

  struct NdLet : Node {
    Token& name;

    NdSymbol* type = nullptr;
    Node* init = nullptr;

    bool is_pub = false; // when field

    int index = 0;

    sema::VariableInfo* var_info_ptr = nullptr;

    NdLet(Token& t, Token& name) : Node(NodeKind::Let, t), name(name) {}
  };

  struct NdIf : Node {
    Node* cond = nullptr;
    Node* thencode = nullptr;
    Node* elsecode = nullptr;
    NdIf(Token& t) : Node(NodeKind::If, t) {}
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

      sema::VariableInfo* var_info_ptr = nullptr;

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

  struct NdModule : Node {
    std::string name;
    std::vector<Node*> items;

    NdFunction* main_fn = nullptr;

    NdModule(Token& tok) : Node(NodeKind::Module, tok) {}
  };
} // namespace superman