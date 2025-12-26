#include "Sema.hpp"

namespace fire {

  Scope* Scope::from_node(Node* node, Scope* parent) {
    switch (node->kind) {
      case NodeKind::Scope:
        return new SCScope(node->as<NdScope>(), parent);
      case NodeKind::If:
        return new SCIf(node->as<NdIf>(), parent);
      case NodeKind::For:
        return new SCFor(node->as<NdFor>(), parent);
      case NodeKind::Catch:
        return new SCCatch(node->as<NdCatch>(), parent);
      case NodeKind::Try:
        return new SCTry(node->as<NdTry>(), parent);
      case NodeKind::Function:
        return new SCFunction(node->as<NdFunction>(), parent);
      case NodeKind::Enum:
        return new SCEnum(node->as<NdEnum>(), parent);
      case NodeKind::Class:
        return new SCClass(node->as<NdClass>(), parent);
      case NodeKind::Namespace:
        return new SCNamespace(node->as<NdNamespace>(), parent);
      case NodeKind::Module:
        return new SCModule(node->as<NdModule>(), parent);
      default:
        todo;
    }
    todo;
  }

  Scope::Scope(ScopeKind kind, Node* node, Scope* parent) : kind(kind), node(node), parent(parent) {
  }

  SCScope::SCScope(NdScope* node, Scope* parent) : Scope(ScopeKind::Scope, node, parent) {
    node->scope_ptr = this;

    for (auto item : node->items) {
      switch (item->kind) {
        case NodeKind::Let: {
          auto let = item->as<NdLet>();

          for (auto s : symtable.symbols) {
            if (s->kind == SymbolKind::Var && s->name == let->name.text) {
              let->symbol_ptr = s;
              alert;
              goto __pass_create_letsym;
            }
          }

          let->symbol_ptr =
              symtable.append(variables.append(Sema::get_instance().new_variable_symbol(let)));

          assert(let->symbol_ptr);

        __pass_create_letsym:;
          break;
        }

        case NodeKind::Scope:
        case NodeKind::For:
        case NodeKind::If:
          subscopes.push_back(Scope::from_node(item, this));
          break;
      }
    }
  }

  SCIf::SCIf(NdIf* node, Scope* parent) : Scope(ScopeKind::If, node, parent) {
    node->scope_ptr = this;

    if (node->vardef) {
      var = Sema::get_instance().new_variable_symbol(node->vardef);
      symtable.append(var);
    }

    then_scope = new SCScope(node->thencode, this);

    if (node->elsecode && node->elsecode->is(NodeKind::Scope)) {
      else_scope = new SCScope(node->elsecode->as<NdScope>(), this);
    }
  }

  SCFor::SCFor(NdFor* node, Scope* parent) : Scope(ScopeKind::For, node, parent) {
    node->scope_ptr = this;

    iter_name = Sema::get_instance().new_variable_symbol(&node->iter, node->iter.text);

    symtable.append(iter_name);

    body = new SCScope(node->body, this);
  }

  SCCatch::SCCatch(NdCatch* node, Scope* parent) : Scope(ScopeKind::Catch, node, parent) {
    node->scope_ptr = this;

    holder_name = Sema::get_instance().new_variable_symbol(&node->holder, node->holder.text);

    symtable.append(holder_name);

    body = new SCScope(node->body, this);

    node->scope_ptr = this;
  }

  SCTry::SCTry(NdTry* node, Scope* parent) : Scope(ScopeKind::Try, node, parent) {
    node->scope_ptr = this;

    body = new SCScope(node->body, this);

    for (auto& catch_node : node->catches) {
      auto cc = new SCCatch(catch_node, this);

      cc->holder_name =
          Sema::get_instance().new_variable_symbol(&catch_node->holder, catch_node->holder.text);

      cc->symtable.append(cc->holder_name);
      cc->body = new SCScope(catch_node->body, cc);

      catches.push_back(cc);
    }

    if (node->finally_block) {
      finally_scope = new SCScope(node->finally_block, this);
    }

    node->scope_ptr = this;
  }

  SCFunction::SCFunction(NdFunction* node, Scope* parent) : Scope(ScopeKind::Func, node, parent) {
    node->scope_ptr = this;

    symbol = {
        .name = std::string(node->name.text),
        .kind = SymbolKind::Func,
        .node = node,
        .scope = this,
    };

    for (auto& arg : node->args) {
      auto a = arguments.append(Sema::get_instance().new_variable_symbol(&arg.name, arg.name.text));
      symtable.append(a);

      arg.var_info_ptr = a->var_info;
    }

    body = new SCScope(node->body, this);

    node->scope_ptr = this;
  }

  SCEnum::SCEnum(NdEnum* node, Scope* parent) : Scope(ScopeKind::Enum, node, parent) {
    node->scope_ptr = this;

    symbol = {
        .name = std::string(node->name.text),
        .kind = SymbolKind::Enum,
        .node = node,
        .scope = this,
    };

    for (auto& enumerator : node->enumerators) {
      auto en_sym = new Symbol();
      en_sym->name = enumerator->name.text;
      en_sym->kind = SymbolKind::Enumerator;
      en_sym->node = enumerator;
      en_sym->scope = nullptr;

      symtable.append(en_sym);
      enumerators.append(en_sym);
    }

    node->scope_ptr = this;
  }

  SCClass::SCClass(NdClass* node, Scope* parent) : Scope(ScopeKind::Class, node, parent) {
    node->scope_ptr = this;

    symbol = {
        .name = std::string(node->name.text),
        .kind = SymbolKind::Class,
        .node = node,
        .scope = this,
    };

    for (auto& field : node->fields) {
      fields.append(Sema::get_instance().new_variable_symbol(field));
    }

    for (auto& method : node->methods) {
      auto scope = new SCFunction(method, this);
      symtable.append(&scope->symbol);
      methods.append(&scope->symbol);
    }
  }

  SCNamespace::SCNamespace(NdNamespace* node, Scope* parent)
      : Scope(ScopeKind::Namespace, node, parent) {
    node->scope_ptr = this;

    symbol = {
        .name = node->name,
        .kind = SymbolKind::Namespace,
        .node = node,
        .scope = this,
    };

    for (auto& item : node->items) {
      if (item->is(NodeKind::Let)) {
        auto let = item->as<NdLet>();
        auto s = variables.append(Sema::get_instance().new_variable_symbol(let));
        symtable.append(s);
        let->symbol_ptr = s;
        assert(let->symbol_ptr);
      } else {
        auto scope = Scope::from_node(item, this);
        symtable.append(&scope->symbol);
        get_table(item->kind)->append(&scope->symbol);
      }
    }
  }

  SCModule::SCModule(NdModule* node, Scope* parent) : Scope(ScopeKind::Module, node, parent) {
    node->scope_ptr = this;

    symbol = {
        .name = node->name,
        .kind = SymbolKind::Module,
        .node = node,
        .scope = this,
    };

    for (auto& item : node->items) {
      if (item->is(NodeKind::Let)) {
        auto let = item->as<NdLet>();
        auto s = variables.append(Sema::get_instance().new_variable_symbol(let));
        symtable.append(s);
        let->symbol_ptr = s;
        assert(let->symbol_ptr);
      } else {
        auto scope = Scope::from_node(item, this);
        symtable.append(&scope->symbol);
        get_table(item->kind)->append(&scope->symbol);
      }
    }
  }
} // namespace fire