#include <iostream>
#include <string>
#include <filesystem>
#include <cstring>

#include "Utils.hpp"

#include "Lexer.hpp"
#include "Token.hpp"
#include "Parser.hpp"
#include "Sema.hpp"
#include "Driver.hpp"
#include "Error.hpp"

namespace fire {

  Driver* __instance = nullptr;

  Driver::Driver() {
    __instance = this;
  }

  Driver* Driver::get_instance() {
    return __instance;
  }

  int Driver::main(int argc, char** argv) {
    this->cwd = std::filesystem::current_path().string();

    bool opt_print_ast = false;
    bool opt_print_tokens = false;
    
    for (int i = 1; i < argc; i++) {
      char const* arg = argv[i];

      if (std::strncmp(arg, "--", 2) == 0) {
        arg += 2;

        if (std::strcmp(arg, "print-ast") == 0) {
          opt_print_ast = true;
        }
        else if(std::strcmp(arg,"print-tokens")==0){
          opt_print_tokens=true;
        }
        else {
          std::cout << "unknown option: " << arg << std::endl;
          return -1;
        }
      } else {
        this->inputs.emplace_back(new SourceFile(arg));
      }
    }

    if (inputs.empty()) {
      std::cout << "no input files." << std::endl;
      return -1;
    }

    for (SourceFile* source : this->inputs) {

      std::filesystem::current_path(source->get_folder());

      try {
        auto tok = source->lex();

        if (opt_print_tokens) {
          for(Token*t=tok;t;t=t->next)
            std::cout<<t->text<<" ";
          std::cout<<std::endl;
        }

        auto mod = source->parse();

        if (opt_print_ast) {
          std::cout << node2s(mod) << std::endl;
        }

        mod->name = "__main__";

        if (!mod->main_fn) {
          printf("fatal error: function 'main' not defined.\n");
          return -1;
        } else if (!mod->main_fn->result_type) {
          printf("fatal error: function 'main' must have a return type.\n");
          return -1;
        }

        Sema::analyze_all(mod);

        // if
        // (!mod->main_fn->scope_ptr->as<FunctionScope>()->result_type.equals(TypeInfo(TypeKind::Int)))
        // {
        //   printf("fatal error: function 'main' must return an int.\n");
        //   return -1;
        // }

        // Compiler::compile_full(IR::from_node(mod));

        return 0;
      }
      catch (int n) {
        printf("%d\n", n);
      }
      catch (err::e& e) {
        e.print();
      }
    }

    return 1;
  }

} // namespace fire