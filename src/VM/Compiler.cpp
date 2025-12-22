#include "Utils/macro.h"
#include "Utils/Strings.hpp"

#include "Sema/Sema.hpp"

#include "VM/Instruction.hpp"
#include "VM/Compiler.hpp"

#include "VM/Interp/Interp.hpp"
#include "VM/Interp/Object.hpp"

namespace fire {

using namespace lexer;
using namespace parser;
using namespace sema;

using vm::Instruction;

std::string Instruction::to_string() const {
  switch (op) {
    case OP_Do:
      return "do " + strings::node2s(expr);

    case OP_Jmpz:
      return "jmpz " + strings::node2s(expr) + ", " + strings::format("_L%zu", label_index);

    case OP_Label:
      return strings::format("_L%zu:", label_index);
  }

  return "nop";
}

namespace vm::compiler {

  Compiler::Compiler(std::vector<Instruction>& out)
    : out(out)
  {
  }

  void Compiler::compile(Node* node) {
    (void)out;
    (void)node;

  }

}

}