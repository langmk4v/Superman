#include <iostream>

#include "Utils.hpp"
#include "Error.hpp"

#include "Sema.hpp"

#include "BuiltinFunc.hpp"

namespace fire {

  static Sema* __inst = nullptr;

  // clang-format off
  static std::pair<std::string, TypeKind> bultin_class_names[] {
    {"none", TypeKind::None},
    {"int", TypeKind::Int},
    {"float", TypeKind::Float},
    {"usize", TypeKind::USize},
    {"bool", TypeKind::Bool},
    {"char", TypeKind::Char},
    {"string", TypeKind::String},
    {"Vec", TypeKind::Vector},
    {"List", TypeKind::List},
    {"tuple", TypeKind::Tuple},
    {"dict", TypeKind::Dict},
    {"functor", TypeKind::Function},
  };
  // clang-format on

  Sema::Sema() {
  }

  Sema& Sema::get_instance() {
    if (!__inst) {
      __inst = new Sema();
    }
    return *__inst;
  }

  void Sema::analyze_all(NdModule* mod) {
    Sema::get_instance().analyze_full(mod);
  }

  void Sema::analyze_full(NdModule* mod) {
    root_scope = new SCModule(mod, nullptr);

    NameResolver resolver(*this);

    alert;
    resolver.on_module(mod, {});

    TypeChecker checker(*this);

    alert;
    checker.check_module(mod, {});
  }

  Symbol* Sema::new_variable_symbol(NdLet* let) {
    auto symbol = new Symbol();

    symbol->name = let->name.text;
    symbol->kind = SymbolKind::Var;
    symbol->node = let;

    symbol->var_info = new VariableInfo();

    return symbol;
  }

  Symbol* Sema::new_variable_symbol(Token* tok, std::string_view name) {
    auto symbol = new Symbol();

    symbol->name = std::string(name);
    symbol->kind = SymbolKind::Var;
    symbol->token = tok;

    symbol->var_info = new VariableInfo();

    return symbol;
  }

  SymbolFindResult Sema::find_symbol(NdSymbol* node, NdVisitorContext ctx) {
    (void)node;
    (void)ctx;

    SymbolFindResult result = {.node = node};

    for (auto scope = ctx.cur_scope; scope; scope = scope->parent) {
      for (auto& symbol : scope->symtable.symbols) {
        if (symbol->name == node->name.text) {
          result.hits.push_back(symbol);
        }
      }

      if (result.hits.size() >= 1)
        break;
    }

    if (result.hits.empty()) {

      for (auto& [name, kind] : bultin_class_names) {
        if (name == node->name.text) {
          result.hits.push_back(new Symbol{
              .name = name,
              .kind = SymbolKind::BuiltinType,
              .type = TypeInfo(kind),
          });
          return result;
        }
      }

      // find builtin funcs
      for (auto& func : builtin_func_table) {
        if (func->name == node->name.text) {
          TypeInfo ty = TypeInfo(TypeKind::Function);
          ty.parameters = func->arg_types;
          ty.parameters.insert(ty.parameters.begin(), func->result_type);
          ty.is_var_arg_functor = func->is_var_args;
          result.hits.push_back( node->symbol_ptr = new Symbol{
              .name = func->name,
              .kind = SymbolKind::BuiltinFunc,
              .type = ty,
              .builtin_f = func,
          });
          return result;
        }
      }

      return result;
    }

    if (node->next) {

      if (result.hits.size() >= 2) {
        throw err::ambiguous_symbol_name(node->token);
      }

      ctx.cur_scope = result.hits[0]->scope;

      switch (ctx.cur_scope->kind) {
        case ScopeKind::Namespace:
        case ScopeKind::Class:
        case ScopeKind::Enum: {
          result = find_symbol(node->next, ctx);
          result.previous = node;
          return result;
        }
      }

      throw err::invalid_scope_resolution(*node->scope_resol_tok);
    }

    return result;
  }

} // namespace fire