#include "Utils/macro.h"
#include "Utils/Strings.hpp"

#include "Sema/Sema.hpp"

#include "VM/Instruction.hpp"
#include "VM/Compiler.hpp"

#include "VM/Interp/Interp.hpp"
#include "VM/Interp/Object.hpp"

namespace fire::vm {

  using namespace lexer;
  using namespace parser;
  using namespace sema;

  std::string Instruction::to_string() const {
    switch (op) {
      case OP_Do:
        return "do " + strings::node2s(expr)+";";

      case OP_Jmp:
        return "jmp " + label;

      case OP_Jmpz:
        return "jmpz " + strings::node2s(expr) + ", " + label;

      case OP_Ret:
        return "ret;";

      case OP_Vardef:
        if(expr) return "var " + std::to_string(var_index) + ", " + strings::node2s(expr)+";";
        return strings::format("var %zu;",var_index);

      case OP_Label:
        return label + ":";
    }

    return "nop;";
  }

  Compiler::Compiler(std::vector<Instruction>& out)
    : out(out)
  {
  }

  void Compiler::compile(Node* node) {
    (void)out;
    (void)node;

    switch(node->kind){
      case NodeKind::Module: {
        auto mod=node->as<NdModule>();
        for(auto item:mod->items){
          compile(item);
        }
        break;
      }

      case NodeKind::Class: {
        auto cla = node->as<NdClass>();

        for(auto&&me:cla->methods){
          me->name.text=cla->name.text+"::"+me->name.text;
          compile(me);
        }

        if(cla->m_new){
          cla->m_new->name.text=cla->name.text+"::new";
          compile(cla->m_new);
        }
        
        break;
      }

      case NodeKind::Function: {

        auto func = node->as<NdFunction>();

        emit({ .op = OP_Label, .label = func->name.text });

        compile(func->body);

        if(out.back().op!=OP_Ret)
          emit({.op=OP_Ret});

        break;
      }

      case NodeKind::Scope: {
        auto const scope = node->as<NdScope>();

        for(auto&&x:scope->items){
          compile(x);
        }

        break;
      }

      case NodeKind::Let: {
        auto const let = node->as<NdLet>();
        emit({ .op = OP_Vardef, .expr = let->init, .var_index = (size_t)let->index });
        break;
      }

      default:
        emit({ .op = OP_Do, .expr = node });
        break;
    }

  }

debug(
  void Compiler::show_all() {
    for(auto&&i:out) {
      if(i.op!=OP_Label) std::cout << "  "; else std::cout<<"\n";
      std::cout<<i.to_string()<<std::endl;
    }
  }
)

  size_t Compiler::emit(Instruction&& i) {
    out.push_back(std::move(i));
    return out.size()-1;
  }

  std::string Compiler::get_label() {
    return strings::format("_L%zu", label_index++);
  }

}