#include <filesystem>

#include "Driver.hpp"
#include "Error.hpp"
#include "FSHelper.hpp"
#include "Parser.hpp"
#include "Utils.hpp"
#include "strconv.hpp"

namespace fire {

NdLet* Parser::ps_let(bool expect_semi) {
  Token& tok = *expect("var");
  NdLet* let = new NdLet(tok, *cur);

  if (eat("(")) {
    do {
      let->placeholders.emplace_back(*expect_ident());
    } while (!is_end() && eat_comma());
    expect(")");
  } else {
    expect_ident();
  }

  if (eat(":"))
    let->type = ps_type_name();
  if (eat("="))
    let->init = ps_expr();
  if (expect_semi)
    expect(";");
  return let;
}
Node* Parser::ps_stmt() {
  auto& tok = *cur;
  if (look("{"))
    return ps_scope();
  if (look("var"))
    return ps_let();
  if (eat("try")) {
    auto x = new NdTry(tok);
    x->body = ps_scope();
    if (!look("catch")) {
      throw err::parses::expected_catch_block(*cur->prev);
    }
    while (!is_end() && eat("catch")) {
      auto catch_ = new NdCatch(tok);
      catch_->holder = *expect_ident();
      expect_colon();
      catch_->error_type = ps_type_name();
      catch_->body = ps_scope();
      x->catches.emplace_back(catch_);
    }
    if (eat("finally")) {
      x->finally_block = ps_scope();
    }
    return x;
  }
  if (eat("if")) {
    auto x = new NdIf(tok);
    if (look("var"))
      x->vardef = ps_let(false);
    if (!x->vardef || eat(";"))
      x->cond = ps_expr();
    x->thencode = ps_scope();
    if (eat("else")) {
      if (look("if"))
        x->elsecode = ps_stmt();
      else
        x->elsecode = ps_scope();
    }
    return x;
  }
  if (eat("while")) {
    auto x = new NdWhile(tok);
    if (look("var"))
      x->vardef = ps_let(false);
    if (!x->vardef || eat(";"))
      x->cond = ps_expr();
    x->body = ps_scope();
    return x;
  }
  if (eat("for")) {
    auto x = new NdFor(tok);
    x->iter = *expect_ident();
    expect("in");
    x->iterable = ps_expr();
    x->body = ps_scope();
    return x;
  }
  if (eat("return")) {
    auto nd = new NdReturn(tok);
    if (!eat(";"))
      nd->expr = ps_expr(), expect(";");
    return nd;
  }
  if (eat("break")) {
    expect(";");
    return new NdBreakOrContinue(NodeKind::Break, tok);
  }
  if (eat("continue")) {
    expect(";");
    return new NdBreakOrContinue(NodeKind::Continue, tok);
  }
  auto x = ps_expr();
  expect(";");
  return x;
}

NdScope* Parser::ps_scope() {
  auto x = new NdScope(*expect("{"));
  if (!eat("}")) {
    while (!is_end()) {
      x->items.emplace_back(ps_stmt());
      if (eat("}"))
        return x;
    }
    throw err::scope_not_terminated(x->token);
  } else
    return x;
}

} // namespace fire