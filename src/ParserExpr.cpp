#include <filesystem>

#include "Driver.hpp"
#include "Error.hpp"
#include "FSHelper.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "strconv.hpp"

namespace fire {

NdSymbol* Parser::ps_type_name() {
  Token& tok = *cur;

  //
  // parse syntax suger for tuple type
  // (T, U, ...) --> tuple<T, U, ...>
  if (eat("(")) {
    NdSymbol* tuple_type = new NdSymbol(tok);

    tuple_type->name.kind = TokenKind::Identifier;
    tuple_type->name.text = "tuple";
    tuple_type->te_args.push_back(ps_type_name());

    expect_comma();

    do {
      tuple_type->te_args.push_back(ps_type_name());
    } while (!is_end() && eat_comma());

    expect(")");

    return tuple_type;
  }

  if (eat("decltype")) {
    expect("(");

    NdDeclType* x = new NdDeclType(tok, ps_expr());

    expect(")");

    NdSymbol* s = new NdSymbol(tok);
    s->dec = x;

    return s;
  }

  return ps_symbol(true);
}

NdSymbol* Parser::ps_symbol(bool as_typename) {
  auto sym = new NdSymbol(*expect_ident());
  Token* save = cur;

  if (eat("<")) {
    try {
      do
        sym->te_args.emplace_back(ps_type_name());
      while (!is_end() && eat(","));
      if (look(">>"))
        cur->text = ">";
      else
        expect(">");
    } catch (...) {
      sym->te_args.clear();
      cur = save;
    }
  }

  if (Token* tok = cur; eat("::")) {
    sym->scope_resol_tok = tok;
    sym->next = ps_symbol();
  }

  if (as_typename) {
    if (eat(":"))
      sym->concept_nd = ps_type_name();
    sym->is_ref = eat("ref");
    sym->is_const = eat("const");
  }

  return sym;
}

Node* Parser::ps_factor() {
  auto& tok = *cur;
  if (eat("(")) {
    auto node = ps_expr();
    if (eat(",")) {
      auto tu = new NdTuple(tok);
      tu->elems.push_back(node);
      do {
        tu->elems.push_back(ps_expr());
      } while (!is_end() && eat(","));
      node = tu;
    }
    expect(")");
    return node;
  }
  if (eat("[")) {
    auto node = new NdArray(tok);
    if (eat("]"))
      return node;
    do {
      node->data.emplace_back(ps_expr());
    } while (eat(","));
    expect("]");
    return node;
  }

  if (eat("self"))
    return new NdOneToken(NodeKind::Self, tok);

  if (eat("nullopt"))
    return new NdOneToken(NodeKind::NullOpt, tok);

  if (eat("true")) {
    auto v = new NdValue(tok);
    v->obj = new ObjBool(true);
    return v;
  }

  if (eat("false")) {
    auto v = new NdValue(tok);
    v->obj = new ObjBool(false);
    return v;
  }

  if (eat("decltype")) {
    throw err::parses::cannot_use_decltype_here(tok);
  }

  if (cur->is(TokenKind::Identifier)) {
    return ps_symbol();
  }

  NdValue* v = new NdValue(*cur);

  switch (cur->kind) {
  case TokenKind::Int:
    v->obj = new ObjInt(std::atoll(cur->text.data()));
    next();
    break;

  case TokenKind::Float:
    v->obj = new ObjFloat(std::atof(cur->text.data()));
    next();
    break;

  case TokenKind::Char: {
    std::u16string s16 =
        utf8_to_utf16_len_cpp(cur->text.data() + 1, cur->text.length() - 2);
    if (s16.empty() || s16.size() > 1)
      throw err::invalid_character_literal(*cur);
    v->obj = new ObjChar(s16[0]);
    next();
    break;
  }

  case TokenKind::String: {
    v->obj = ObjString::from_char16_ptr_move(utf8_to_utf16_with_len(
        nullptr, cur->text.data() + 1, cur->text.length() - 2));
    next();
    break;
  }

  default:
    throw err::invalid_syntax(*cur);
  }

  return v;
}

Node* Parser::ps_subscript() {
  auto& tok = *cur;
  auto x = ps_factor();
  while (!is_end()) {
    auto op = cur;
    if (eat_paren_open()) {
      auto y = new NdCallFunc(x, *op);
      if (x->is(NodeKind::MemberAccess)) {
        y->is_method_call = true;
        y->inst_expr = x->as<NdExpr>()->lhs;
        y->callee = x->as<NdExpr>()->rhs;
      }
      if (!eat_paren_close()) {
        do {
          auto key = ps_expr();
          if (eat_colon())
            y->args.push_back(new NdKeyValuePair(*op, key, ps_expr()));
          else
            y->args.push_back(key);
        } while (eat_comma());
        expect_paren_close();
      }
      x = y;
    } else if (eat_bracket_open()) {
      // "[:end]"
      if (eat_colon()) {
        auto end = ps_expr();
        x = new NdExpr(NodeKind::Subscript, tok, x,
                       new NdExpr(NodeKind::Slice, tok, nullptr, end));
        expect_bracket_close();
      } else {
        auto index = ps_expr();
        if (eat_colon()) {
          auto end = look("]") ? nullptr : ps_expr();
          x = new NdExpr(NodeKind::Subscript, tok, x,
                         new NdExpr(NodeKind::Slice, tok, index, end));
        } else
          x = new NdExpr(NodeKind::Subscript, tok, x, index);
        expect_bracket_close();
      }
    } else if (eat_dot()) {
      // get tuple element:
      // a.<N>
      if (eat_angle_open()) {
        if (cur->kind != TokenKind::Int) {
          throw err::expected_but_found(*cur, "int");
        }
        x = new NdGetTupleElement(tok, x, std::atoi(cur->text.data()));
        x->as<NdGetTupleElement>()->index_tok = cur;
        next();
        expect_angle_close();
        continue;
      } else {
        auto right = ps_factor();
        x = new NdExpr(NodeKind::MemberAccess, *op, x, right);
      }
    } else
      break;
  }

  if (eat(TokenPunctuators::Punct_Inclement))
    return new NdInclement(tok, x, false);

  if (eat(TokenPunctuators::Punct_Declement))
    return new NdDeclement(tok, x, false);

  return x;
}

Node* Parser::ps_unary() {
  auto& tok = *cur;

  if (eat(TokenPunctuators::Punct_Inclement))
    return new NdInclement(tok, ps_subscript(), true);

  if (eat(TokenPunctuators::Punct_Declement))
    return new NdDeclement(tok, ps_subscript(), true);

  if (eat(TokenPunctuators::Punct_BitAnd))
    return new NdOneExprWrap(NodeKind::Ref, tok, ps_subscript());

  if (eat(TokenPunctuators::Punct_Mul))
    return new NdOneExprWrap(NodeKind::Deref, tok, ps_subscript());

  if (eat(TokenPunctuators::Punct_Not))
    return new NdOneExprWrap(NodeKind::Not, tok, ps_subscript());

  if (eat(TokenPunctuators::Punct_Tilde))
    return new NdOneExprWrap(NodeKind::BitNot, tok, ps_subscript());

  if (eat(TokenPunctuators::Punct_Sub)) {
    auto zero = new NdValue(tok);
    zero->obj = new ObjInt(0);
    return new NdExpr(NodeKind::Sub, tok, zero, ps_subscript());
  }

  eat(TokenPunctuators::Punct_Add);
  return ps_subscript();
}

Node* Parser::ps_terms() {
  auto x = ps_unary();
  while (!is_end()) {
    auto op = cur;
    if (eat(TokenPunctuators::Punct_Mul))
      x = new NdExpr(NodeKind::Mul, *op, x, ps_unary());
    else if (eat(TokenPunctuators::Punct_Div))
      x = new NdExpr(NodeKind::Div, *op, x, ps_unary());
    else
      break;
  }
  return x;
}

Node* Parser::ps_add_sub() {
  auto x = ps_terms();
  while (!is_end()) {
    auto op = cur;
    if (eat(TokenPunctuators::Punct_Add))
      x = new NdExpr(NodeKind::Add, *op, x, ps_terms());
    else if (eat(TokenPunctuators::Punct_Sub))
      x = new NdExpr(NodeKind::Sub, *op, x, ps_terms());
    else
      break;
  }
  return x;
}

Node* Parser::ps_shift() {
  auto x = ps_add_sub();
  while (!is_end()) {
    auto op = cur;
    if (eat(TokenPunctuators::Punct_LShift))
      x = new NdExpr(NodeKind::LShift, *op, x, ps_add_sub());
    else if (eat(TokenPunctuators::Punct_RShift))
      x = new NdExpr(NodeKind::RShift, *op, x, ps_add_sub());
    else
      break;
  }
  return x;
}

Node* Parser::ps_compare() {
  auto x = ps_shift();
  while (!is_end()) {
    auto op = cur;
    if (eat(TokenPunctuators::Punct_Less))
      x = new NdExpr(NodeKind::Bigger, *op, ps_shift(), x);
    else if (eat(TokenPunctuators::Punct_Greater))
      x = new NdExpr(NodeKind::BiggerOrEqual, *op, x, ps_shift());
    else if (eat(TokenPunctuators::Punct_LessOrEqual))
      x = new NdExpr(NodeKind::Bigger, *op, ps_shift(), x);
    else if (eat(TokenPunctuators::Punct_GreaterOrEqual))
      x = new NdExpr(NodeKind::BiggerOrEqual, *op, x, ps_shift());
    else
      break;
  }
  return x;
}

Node* Parser::ps_equality() {
  auto x = ps_compare();
  while (!is_end()) {
    auto op = cur;
    if (eat(TokenPunctuators::Punct_Equal))
      x = new NdExpr(NodeKind::Equal, *op, x, ps_compare());
    else if (eat(TokenPunctuators::Punct_NotEqual))
      x = new NdExpr(NodeKind::Not, *op,
                     new NdExpr(NodeKind::Equal, *op, x, ps_compare()),
                     nullptr);
    else
      break;
  }
  return x;
}

Node* Parser::ps_bit_and() {
  auto x = ps_equality();
  while (!is_end()) {
    auto op = cur;
    if (eat(TokenPunctuators::Punct_BitAnd))
      x = new NdExpr(NodeKind::BitAnd, *op, x, ps_equality());
    else
      break;
  }
  return x;
}

Node* Parser::ps_bit_xor() {
  auto x = ps_bit_and();
  while (!is_end()) {
    auto op = cur;
    if (eat(TokenPunctuators::Punct_BitXor))
      x = new NdExpr(NodeKind::BitXor, *op, x, ps_bit_and());
    else
      break;
  }
  return x;
}

Node* Parser::ps_bit_or() {
  auto x = ps_bit_xor();
  while (!is_end()) {
    auto op = cur;
    if (eat(TokenPunctuators::Punct_BitOr))
      x = new NdExpr(NodeKind::BitOr, *op, x, ps_bit_xor());
    else
      break;
  }
  return x;
}

Node* Parser::ps_log_and() {
  auto x = ps_bit_or();
  while (!is_end() && eat(TokenPunctuators::Punct_And))
    x = new NdExpr(NodeKind::LogAnd, *cur->prev, x, ps_bit_or());
  return x;
}

Node* Parser::ps_log_or() {
  auto x = ps_log_and();
  while (!is_end() && eat(TokenPunctuators::Punct_Or))
    x = new NdExpr(NodeKind::LogOr, *cur->prev, x, ps_log_and());
  return x;
}

Node* Parser::ps_assign() {
  auto x = ps_log_or();
  auto op = cur;
  if (eat(TokenPunctuators::Punct_Assign))
    x = new NdExpr(NodeKind::Assign, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_AddAssign))
    x = new NdAssignWithOp(NodeKind::Add, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_SubAssign))
    x = new NdAssignWithOp(NodeKind::Sub, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_MulAssign))
    x = new NdAssignWithOp(NodeKind::Mul, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_DivAssign))
    x = new NdAssignWithOp(NodeKind::Div, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_ModAssign))
    x = new NdAssignWithOp(NodeKind::Mod, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_AndAssign))
    x = new NdAssignWithOp(NodeKind::BitAnd, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_OrAssign))
    x = new NdAssignWithOp(NodeKind::BitOr, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_XorAssign))
    x = new NdAssignWithOp(NodeKind::BitXor, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_LShiftAssign))
    x = new NdAssignWithOp(NodeKind::LShift, *op, x, ps_assign());
  if (eat(TokenPunctuators::Punct_RShiftAssign))
    x = new NdAssignWithOp(NodeKind::RShift, *op, x, ps_assign());
  return x;
}

Node* Parser::ps_expr() { return ps_assign(); }

} // namespace fire