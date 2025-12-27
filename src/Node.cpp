#include "Token.hpp"
#include "Node.hpp"

// only defines all ctor and dtor in this file.

namespace fire {

Node::Node(NodeKind k, Token const& t) : kind(k), token(t), text(t.text) {}
Node::Node(NodeKind kind, std::string const& text) : kind(kind), text(text) {}

NdKeyValuePair::NdKeyValuePair(Token const& t, Node* key, Node* value)
    : Node(NodeKind::KeyValuePair, t), key(key), value(value) {}
NdKeyValuePair::~NdKeyValuePair() {
  if (key) delete key;
  if (value) delete value;
}

NdValue::NdValue(Token const& t) : Node(NodeKind::Value, t) {}
NdValue::NdValue(Token const& t, Object* obj)
    : Node(NodeKind::Value, t), obj(obj) {}
NdValue::~NdValue() {
  if (obj) delete obj;
}

NdDeclType::NdDeclType(Token const& tok, Node* expr)
    : Node(NodeKind::DeclType, tok), expr(expr) {}
NdDeclType::~NdDeclType() {
  if (expr) delete expr;
}

NdSymbol::NdSymbol(NdDeclType* de)
    : Node(NodeKind::Symbol, de->token), name(de->token), dec(de) {}

NdSymbol::NdSymbol(Token const& t, NdDeclType* de)
    : Node(NodeKind::Symbol, t), name(token), dec(de) {}

NdSymbol::~NdSymbol() {
  if (dec) delete dec;
  for (auto arg : te_args)
    delete arg;
}

NdOneToken::NdOneToken(NodeKind kind, Token const& t) : Node(kind, t) {}
NdOneToken::~NdOneToken() {}

NdArray::NdArray(Token const& t) : Node(NodeKind::Array, t) {}
NdArray::~NdArray() {
  for (auto elem : data)
    delete elem;
}

NdTuple::NdTuple(Token const& t) : Node(NodeKind::Tuple, t) {}
NdTuple::~NdTuple() {
  for (auto elem : elems)
    delete elem;
}

NdCallFunc::NdCallFunc(Node* callee, Token const& tok)
    : Node(NodeKind::CallFunc, tok), callee(callee) {}
NdCallFunc::~NdCallFunc() {
  if (callee) delete callee;
  for (auto arg : args)
    delete arg;
}

NdGetTupleElement::NdGetTupleElement(Token const& tok, Node* expr, int index)
    : Node(NodeKind::GetTupleElement, tok), expr(expr), index(index) {}
NdGetTupleElement::~NdGetTupleElement() {
  if (expr) delete expr;
}

NdInclementDeclement::NdInclementDeclement(NodeKind k, Token const& tok,
                                           Node* expr, bool is_postfix)
    : Node(k, tok), expr(expr), is_postfix(is_postfix) {}
NdInclementDeclement::~NdInclementDeclement() {
  if (expr) delete expr;
}

NdOneExprWrap::NdOneExprWrap(NodeKind k, Token const& t, Node* e)
    : Node(k, t), expr(e) {}
NdOneExprWrap::~NdOneExprWrap() {
  if (expr) delete expr;
}

NdAssignWithOp::NdAssignWithOp(NodeKind opkind, Token& op, Node* l, Node* r)
    : Node(NodeKind::AssignWithOp, op), opkind(opkind), lhs(l), rhs(r) {}
NdAssignWithOp::~NdAssignWithOp() {
  if (lhs) delete lhs;
  if (rhs) delete rhs;
}

NdExpr::NdExpr(NodeKind k, Token& op, Node* l, Node* r)
    : Node(k, op), lhs(l), rhs(r) {}
NdExpr::~NdExpr() {
  if (lhs) delete lhs;
  if (rhs) delete rhs;
}

NdLet::NdLet(Token const& t, Token const& name)
    : Node(NodeKind::Let, t), name(name) {}
NdLet::~NdLet() {
  if (type) delete type;
  if (init) delete init;
}

NdCatch::NdCatch(Token const& t) : Node(NodeKind::Catch, t) {}
NdCatch::~NdCatch() {
  if (error_type) delete error_type;
  delete body;
}

NdTry::NdTry(Token const& t) : Node(NodeKind::Try, t) {}
NdTry::~NdTry() {
  delete body;
  for (auto catch_ : catches)
    delete catch_;
  if (finally_block) delete finally_block;
}

NdIf::NdIf(Token const& t) : Node(NodeKind::If, t) {}
NdIf::~NdIf() {
  if (vardef) delete vardef;
  if (cond) delete cond;
  delete thencode;
  if (elsecode) delete elsecode;
}

NdFor::NdFor(Token const& t) : Node(NodeKind::For, t) {}
NdFor::~NdFor() {
  if (iterable) delete iterable;
  delete body;
}

NdWhile::NdWhile(Token const& t) : Node(NodeKind::While, t) {}
NdWhile::~NdWhile() {
  if (vardef) delete vardef;
  if (cond) delete cond;
  delete body;
}

NdReturn::NdReturn(Token const& t) : Node(NodeKind::Return, t) {}
NdReturn::~NdReturn() {
  if (expr) delete expr;
}

NdBreakOrContinue::NdBreakOrContinue(NodeKind k, Token& t) : Node(k, t) {}

NdBreakOrContinue::~NdBreakOrContinue() {}

NdScope::NdScope(Token const& t) : Node(NodeKind::Scope, t) {}
NdScope::~NdScope() {
  for (auto item : items)
    delete item;
}

NdTemplatableBase::NdTemplatableBase(NodeKind k, Token const& t) : Node(k, t) {}
NdTemplatableBase::~NdTemplatableBase() {
  for (auto param : parameter_defs)
    delete param;
}

NdFuncArgument::NdFuncArgument(Token const& n, NdSymbol* type)
    : Node(NodeKind::FuncArgument, n), name(n), type(type) {}
NdFuncArgument::~NdFuncArgument() {
  if (type) delete type;
}

NdFunction::NdFunction(Token const& t, Token const& name)
    : NdTemplatableBase(NodeKind::Function, t), name(name) {}
NdFunction::~NdFunction() {
  for (auto arg : args)
    delete arg;
  if (result_type) delete result_type;
  delete body;
}

NdEnum::NdEnum(Token const& t) : Node(NodeKind::Enum, t) {}
NdEnum::~NdEnum() {
  for (auto e : enumerators)
    delete e;
}

NdEnumeratorDef::NdEnumeratorDef(Token const& t)
    : Node(NodeKind::EnumeratorDef, t) {}
NdEnumeratorDef::~NdEnumeratorDef() {
  if (variant) delete variant;
  for (auto n : multiple)
    delete n;
}

NdClass::NdClass(Token const& tok, Token& name)
    : NdTemplatableBase(NodeKind::Class, tok), name(name) {}
NdClass::~NdClass() {
  for (auto m : methods)
    delete m;
  for (auto f : fields)
    delete f;
}

NdNamespace::NdNamespace(Token const& tok, std::string const& name)
    : Node(NodeKind::Namespace, tok), name(name) {}
NdNamespace::~NdNamespace() {
  for (auto n : items)
    delete n;
}

NdModule::NdModule(Token const& tok) : Node(NodeKind::Module, tok) {}
NdModule::~NdModule() {
  for (auto n : items)
    delete n;
}

} // namespace fire