#include "Utils.hpp"
#include "Node.hpp"

namespace fire {

  static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conv;

  std::string to_utf8(std::u16string const& s) { return conv.to_bytes(s); }

  std::u16string to_utf16(std::string const& s) { return conv.from_bytes(s); }

  std::string node2s(Node* node) {
    static int indent = 0;

    auto ind = std::string(indent * 2, ' ');

    if (!node) return "<null>";

    switch (node->kind) {
    case NodeKind::Value:
      if (node->token.kind == TokenKind::Char) return "'" + node->token.text + "'";
      if (node->token.kind == TokenKind::String) return "\"" + node->token.text + "\"";
      return node->token.text;

    case NodeKind::Self:
      return "self";

    case NodeKind::Symbol: {
      auto x = node->as<NdSymbol>();
      auto s = x->name.text;
      if (x->te_args.size() >= 1) s += "<" + join(", ", x->te_args, node2s) + ">";
      if (x->next) s += "::" + node2s(x->next);
      return s;
    }

    case NodeKind::CallFunc: {
      auto x = node->as<NdCallFunc>();
      return node2s(x->callee) + "(" + join(", ", x->args, node2s) + ")";
    }

    case NodeKind::MemberAccess: {
      auto x = node->as<NdExpr>();
      return node2s(x->lhs) + "." + node2s(x->rhs);
    }

    case NodeKind::New: {
      auto x = node->as<NdNew>();
      return "new " + node2s(x->type) + "(" + join(", ", x->args, node2s) + ")";
    }

    case NodeKind::Scope: {
      auto x = node->as<NdScope>();

      indent++;
      auto s = "{\n" + std::string(indent * 2, ' ');

      s += join("\n" + std::string(indent * 2, ' '), x->items, node2s);
      s += "\n" + ind + "}";

      indent--;
      return s;
    }

    case NodeKind::Function: {
      auto x = node->as<NdFunction>();

      auto s =
          "fn " + x->name.text + " (" +
          join(", ", x->args,
               [](NdFunction::Argument const& arg) -> std::string { return arg.name.text + ": " + node2s(arg.type); }) +
          ") -> " + node2s(x->result_type) + " " + node2s(x->body);

      return s;
    }

    case NodeKind::Module: {
      auto x = node->as<NdModule>();
      return join("\n", x->items, node2s);
    }

    case NodeKind::Return: {
      auto x = node->as<NdReturn>();
      return x->expr ? "return " + node2s(x->expr) + ";" : "return;";
    }
    }

    if (node->is_expr()) {
      auto ex = node->as<NdExpr>();
      return node2s(ex->lhs) + " " + ex->token.text + " " + node2s(ex->rhs);
    }

    return "???";
  }

} // namespace fire
