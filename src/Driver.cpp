#include <iostream>
#include <string>
#include <filesystem>

#include "Utils.hpp"

#include "Lexer.hpp"
#include "Token.hpp"
#include "Parser.hpp"
#include "Sema.hpp"
#include "VM.hpp"
#include "Driver.hpp"
#include "Error.hpp"

#include "IR.hpp"

namespace fire {

  Driver* __instance = nullptr;

  Driver::Driver() { __instance = this; }

  Driver* Driver::get_instance() { return __instance; }

  int Driver::main(int argc, char** argv) {
    this->cwd = std::filesystem::current_path().string();

    for (int i = 1; i < argc; i++)
      this->inputs.emplace_back(new SourceCode(argv[i]));

    if (inputs.empty()) {
      std::cout << "no input files." << std::endl;
      return -1;
    }

    for (auto source : this->inputs) {

      std::filesystem::current_path(source->get_folder());

      try {
        auto mod = source->parse();

        mod->name = "__main__";

        if (!mod->main_fn) {
          printf("fatal error: function 'main' not defined.\n");
          return -1;
        } else if (!mod->main_fn->result_type) {
          printf("fatal error: function 'main' must have a return type.\n");
          return -1;
        }

        Sema::analyze_all(mod);

        if (!mod->main_fn->scope_ptr->as<FunctionScope>()->result_type.equals(TypeInfo(TypeKind::Int))) {
          printf("fatal error: function 'main' must return an int.\n");
          return -1;
        }

        Compiler::compile_full(IR::from_node(mod));

        return 0;
      } catch (int n) {
        printf("%d\n", n);
      } catch (err::e e) {
        e.print();
      }
    }

    return 1;
  }

} // namespace fire