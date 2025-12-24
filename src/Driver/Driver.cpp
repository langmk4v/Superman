#include <iostream>

#include <string>

#include "Utils/macro.h"
#include "Utils/Strings.hpp"

#include "Lexer/Source.hpp"
#include "Lexer/Token.hpp"
#include "Lexer/Lexer.hpp"

#include "Parser/Node.hpp"
#include "Parser/Parser.hpp"

#include "Sema/Sema.hpp"

#include "VM/Compiler.hpp"
#include "VM/Instruction.hpp"
#include "VM/Interp/Interp.hpp"

#include "Driver/Driver.hpp"
#include "Driver/Error.hpp"

namespace fire {

  using namespace parser;
  using namespace lexer;

  Driver::Driver() {}

  Driver::~Driver() {}

  int Driver::main(int argc, char** argv) {
    for (int i = 1; i < argc; i++)
      this->inputs.emplace_back(argv[i]);

    if (inputs.empty()) {
      std::cout << "no input files." << std::endl;
      return -1;
    }

    for (auto& source : this->inputs) {
      std::vector<Token> tokens;

      try {
        auto lexer = Lexer(source);
        tokens = lexer.lex();

        auto parser = Parser(lexer, tokens);

        auto mod = parser.parse();

        mod->name = "__main__";

        if (!mod->main_fn) {
          printf("fatal error: function 'main' not defined.\n");
          return -1;
        } else if (mod->main_fn->args.size() > 0) {
          printf("fatal error: function 'main' cannot have arguments.\n");
          return -1;
        } else if (mod->main_fn->result_type) {
          printf("fatal error: result type of 'main' cannot be specified.\n");
          return -1;
        }

        auto se = sema::Sema(mod);
        se.analyze_full();

        std::vector<vm::Instruction> prg;
        std::vector<vm::StructDef*> structs;

        prg.push_back({ .op = vm::OP_Jmp, .label = "main" });

        auto comp = vm::Compiler(prg, structs);

        comp.compile(mod);

      #if _FIRE_DEBUG_
        comp.show_all();
      #endif

        auto runner = vm::interp::Interp(prg);

        runner.run();

        return 0;
      } catch (int n) {
        printf("%d\n", n);
      } catch (err::e e) {
        e.print();
      }
    }

    return 1;
  }

} // namespace superman