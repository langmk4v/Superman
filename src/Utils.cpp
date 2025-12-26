#include <sstream>
#include <codecvt>
#include <locale>

#include "Utils.hpp"
#include "Node.hpp"
#include "Sema.hpp"

namespace fire {

  using std::string_literals::operator""s;
  using string = std::string;

  static std::string template_params_2s(NdTemplatableBase* tpbase) {
    if (tpbase->count() == 0) { return ""; }

    std::stringstream ss;
    ss << " <" << join(", ", tpbase->parameter_defs, node2s) << "> ";
    return ss.str();
  }

  std::string node2s(Node* node) {
    static int indent = 0;

    if (!node) return "";

    std::string const ind(indent * 2, ' ');

    switch (node->kind) {

      case NodeKind::Self:
        return "self";

      case NodeKind::Value: {
        std::stringstream ss;
        if (node->token.kind == TokenKind::Char) {
          ss << "'" << node->token.text << "'";
        } else if (node->token.kind == TokenKind::String) {
          ss << "\"" << node->token.text << "\"";
        } else {
          ss << node->token.text;
        }
        return ss.str();
      }

      case NodeKind::Symbol: {
        auto x = node->as<NdSymbol>();
        std::stringstream ss;

        ss << x->name.text;

        if (!x->te_args.empty()) { ss << "<" << join(", ", x->te_args, node2s) << ">"; }
        if (x->next) { ss << "::" << node2s(x->next); }
        return ss.str();
      }

      case NodeKind::KeyValuePair: {
        auto x = node->as<NdExpr>();
        std::stringstream ss;
        ss << node2s(x->lhs) << ": " << node2s(x->rhs);
        return ss.str();
      }

      case NodeKind::Array: {
        auto x = node->as<NdArray>();
        std::stringstream ss;
        ss << "[" << join(", ", x->data, node2s) << "]";
        return ss.str();
      }

      case NodeKind::Tuple: {
        auto x = node->as<NdTuple>();
        std::stringstream ss;
        ss << "(" << join(", ", x->elems, node2s) << ")";
        return ss.str();
      }

      case NodeKind::CallFunc: {
        auto x = node->as<NdCallFunc>();
        std::stringstream ss;
        ss << node2s(x->callee) << "(" << join(", ", x->args, node2s) << ")";
        return ss.str();
      }

      case NodeKind::MemberAccess: {
        auto x = node->as<NdExpr>();
        std::stringstream ss;
        ss << node2s(x->lhs) << "." << node2s(x->rhs);
        return ss.str();
      }

      case NodeKind::New: {
        auto x = node->as<NdNew>();
        std::stringstream ss;
        ss << "new " << node2s(x->type) << "(" << join(", ", x->args, node2s) << ")";
        return ss.str();
      }

      case NodeKind::Not:
        return "!" + node2s(node->as<NdNot>()->expr);

      case NodeKind::Ref:
        return "&" + node2s(node->as<NdRef>()->expr);

      case NodeKind::Deref:
        return "*" + node2s(node->as<NdDeref>()->expr);

      case NodeKind::Scope: {
        auto x = node->as<NdScope>();
        indent++;

        std::stringstream ss;
        ss << "{";

        for (auto y : x->items) {
          ss << "\n  " << ind << node2s(y);
          if (y->is_expr_full()) ss << ";";
        }

        ss << "\n" << ind << "}";

        indent--;
        return ss.str();
      }

      case NodeKind::Let: {
        auto x = node->as<NdLet>();
        std::stringstream ss;
        ss << "var ";
        if (x->placeholders.size() >= 1) {
          ss << "(" << join(", ", x->placeholders, [](Token* t) { return t->text; }) << ")";
        } else
          ss << x->name.text;
        if (x->type) { ss << " : " << node2s(x->type); }
        if (x->init) { ss << " = " << node2s(x->init); }
        ss << ";";
        return ss.str();
      }

      case NodeKind::If: {
        auto x = node->as<NdIf>();
        std::stringstream ss;
        ss << "if " << node2s(x->vardef) << node2s(x->cond) << " " << node2s(x->thencode);
        if (x->elsecode) ss << "\n" << ind << "else " << node2s(x->elsecode);
        return ss.str();
      }

      case NodeKind::For: {
        auto x = node->as<NdFor>();
        std::stringstream ss;
        ss << "for " << x->iter.text << " " << node2s(x->iterable) << " " << node2s(x->body);
        return ss.str();
      }

      case NodeKind::While: {
        auto x = node->as<NdWhile>();
        std::stringstream ss;
        ss << "while ";
        if (x->vardef) ss << node2s(x->vardef);
        if (x->cond) { ss << node2s(x->cond); }
        ss << " " << node2s(x->body);
        return ss.str();
      }

      case NodeKind::Try: {
        auto x = node->as<NdTry>();
        std::stringstream ss;

        ss << "try " << node2s(x->body);
        for (auto&& c : x->catches) {
          ss << "\n"
             << ind << "catch " << c->holder.text << ": " << node2s(c->error_type) << " "
             << node2s(c->body);
        }
        return ss.str();
      }

      case NodeKind::Function: {
        auto x = node->as<NdFunction>();
        std::stringstream ss;

        ss << "fn " << x->name.text << template_params_2s(x);

        auto args = join(", ", x->args, [](NdFunction::Argument const& arg) {
          std::stringstream a;
          a << arg.name.text << ": " << node2s(arg.type);
          return a.str();
        });

        if (x->take_self) { args = "self"s + (args.empty() ? "" : ", ") + args; }

        ss << "(" << args << ") -> " << node2s(x->result_type) << " " << node2s(x->body);

        return ss.str();
      }

      case NodeKind::Enum: {
        auto x = node->as<NdEnum>();
        std::stringstream ss;
        ss << "enum " << x->name.text << " {\n"
           << ind << "  " << join(",\n" + ind + "  ", x->enumerators, node2s) << "\n"
           << ind << "}";
        return ss.str();
      }

      case NodeKind::EnumeratorDef: {
        auto x = node->as<NdEnumeratorDef>();
        std::stringstream ss;
        if (x->type == NdEnumeratorDef::NoVariants) {
          ss << x->name.text;
        } else {
          ss << x->name.text << "(";
          if (x->type==NdEnumeratorDef::MultipleTypes || x->type==NdEnumeratorDef::StructFields) {
            ss << join(", ", x->multiple, node2s);
          } else {
            ss << node2s(x->variant);
          }
          ss << ")";
        }
        return ss.str();
      }

      case NodeKind::Class: {
        auto x = node->as<NdClass>();
        indent++;

        std::stringstream ss;
        ss << "class " << x->name.text << " {\n"
           << ind << "  " << join("\n" + ind + "  ", x->fields, node2s) << "\n"
           << (x->methods.empty()
                   ? ""
                   : (ind + "  " + join("\n" + ind + "  ", x->methods, node2s) + "\n"))
           << ind << "}";

        indent--;
        return ss.str();
      }

      case NodeKind::Namespace: {
        auto x = node->as<NdNamespace>();
        indent++;

        std::stringstream ss;
        ss << "namespace " << x->name << " {\n  " << join("\n  ", x->items, node2s) << "\n}";

        indent--;
        return ss.str();
      }

      case NodeKind::Module: {
        auto x = node->as<NdModule>();
        return join("\n", x->items, node2s);
      }

      case NodeKind::Return: {
        auto x = node->as<NdReturn>();
        std::stringstream ss;
        ss << "return";
        if (x->expr) { ss << " " << node2s(x->expr); }
        ss << ";";
        return ss.str();
      }
    }

    if (node->is_expr()) {
      auto ex = node->as<NdExpr>();
      std::stringstream ss;
      ss << node2s(ex->lhs) << " " << ex->token.text << " " << node2s(ex->rhs);
      return ss.str();
    }

    return "<"s + node->token.text + ">";
  }

} // namespace fire
