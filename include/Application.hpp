#pragma once

#include <vector>
#include <iostream>

#include "macro.h"
#include "Strings.hpp"

#include "Lexer.hpp"
#include "Parser.hpp"
#include "Sema.hpp"
#include "Eval.hpp"

namespace superman {
  struct SourceCode;
  class Application {
    std::vector<SourceCode> inputs;

  public:
    Application() {}

    ~Application() {}

    int main(int argc, char** argv) {
      for (int i = 1; i < argc; i++)
        this->inputs.emplace_back(argv[i]);

      for (auto& source : this->inputs) {
        std::vector<Token> tokens;

        try {
          auto lexer = Lexer(source);
          tokens = lexer.lex();
          // for (auto&&t:tokens)std::cout<<t.text<<' ';std::cout<<std::endl;

          auto parser = Parser(lexer, tokens);
          auto node = parser.parse();

          node->name = "__main__";

          // std::cout << node2s(node) << std::endl;

          auto se = sema::Sema(node);
          se.check_full();

          auto ev = Evaluator();

          // auto obj = ev.eval_node(node);
          for(auto&& x: node->items)
            ev.eval_node(x);

          return 0;
        } catch (int n) {
          printf("%d\n", n);
        } catch (err::e e) {
          e.print();
        }
      }

      return 1;
    }

  private:
    static std::string node2s(Node* node) {
      static int indent = 0;

      auto ind = std::string(indent * 2, ' ');

      if (!node) return "<null>";

      switch (node->kind) {
      case NodeKind::Value:
        if (node->token.kind == TokenKind::Char) return "'" + node->token.text + "'";
        if (node->token.kind == TokenKind::String) return "\"" + node->token.text + "\"";
        return node->token.text;

      case NodeKind::Symbol: {
        auto x = node->as<NdSymbol>();
        auto s = x->name.text;
        if (x->te_args.size() >= 1) s += "<" + strings::join(", ", x->te_args, node2s) + ">";
        if (x->next) s += "::" + node2s(x->next);
        return s;
      }

      case NodeKind::CallFunc: {
        auto x = node->as<NdCallFunc>();
        return node2s(x->callee) + "(" + strings::join(", ", x->args, node2s) + ")";
      }

      case NodeKind::Scope: {
        auto x = node->as<NdScope>();

        indent++;
        auto s = "{\n" + std::string(indent * 2, ' ');

        s += strings::join("\n" + std::string(indent * 2, ' '), x->items, node2s);
        s += "\n" + ind + "}";

        indent--;
        return s;
      }

      case NodeKind::Function: {
        auto x = node->as<NdFunction>();

        auto s = "fn " + x->name.text + " (" +
                 strings::join(", ", x->args,
                               [](NdFunction::Argument const& arg) -> std::string {
                                 return arg.name.text + ": " + node2s(arg.type);
                               }) +
                 ") -> " + node2s(x->result_type) + " " + node2s(x->body);

        return s;
      }

      case NodeKind::Module: {
        auto x = node->as<NdModule>();
        return strings::join("\n", x->items, node2s);
      }

      case NodeKind::Return: {
        auto x = node->as<NdReturn>();
        return x->expr ? "return " + node2s(x->expr) + ";" : "return;";
      }

      default:
        todoimpl;
      }
    }
  };
} // namespace superman