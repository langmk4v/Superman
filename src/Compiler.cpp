#include "Utils.hpp"
#include "VM.hpp"
#include "Node.hpp"

namespace fire {

  std::string Instruction::to_string() const {
    switch (op) {
    case OP_Do:
      return "do " + node2s(expr) + ";";

    case OP_Jmp:
      return "jmp " + label;

    case OP_Jmpz:
      return "jmpz " + node2s(expr) + ", " + label;

    case OP_Ret:
      return expr ? "ret " + node2s(expr) + ";" : "ret;";

    case OP_Vardef:
      if (expr) return "var " + var_name + " = " + node2s(expr) + ";";
      return "var " + var_name + ";";

    case OP_Label:
      return label + var_name + ":";
    }

    return "nop;";
  }

} // namespace fire