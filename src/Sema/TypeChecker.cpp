#include "Utils.hpp"
#include "Error.hpp"
#include "Node.hpp"
#include "Sema/Sema.hpp"

namespace fire {

  TypeInfo TypeChecker::eval_expr_ty(Node* node, NdVisitorContext ctx) {
    (void)node;
    (void)ctx;

    switch (node->kind) {
      case NodeKind::Value:
        return node->as<NdValue>()->obj->type;

      case NodeKind::Symbol: {
        auto sym = node->as<NdSymbol>();

        if (!sym->symbol_ptr) { todo; }

        switch (sym->symbol_ptr->kind) {
          case SymbolKind::Enum:
          case SymbolKind::Class:
          case SymbolKind::BuiltinType:
          case SymbolKind::TemplateParam:
            return eval_typename_ty(sym, ctx);

          case SymbolKind::Var:
            if (!sym->symbol_ptr->var_info->is_type_deducted) { todo; }
            return sym->symbol_ptr->var_info->type;

          case SymbolKind::Func:
            todo;

          case SymbolKind::Enumerator: {
            auto en = sym->symbol_ptr->node->as<NdEnumeratorDef>();

            assert(en->kind == NodeKind::EnumeratorDef);

            if (ctx.enumerator_node_out) { *ctx.enumerator_node_out = en; }

            return make_enum_type(en->parent_enum_node);
          }

          case SymbolKind::Namespace:
            todo;

          case SymbolKind::Module:
            todo;

          case SymbolKind::BuiltinFunc:
            todo;
        }

        todo; // ???
      }

      case NodeKind::KeyValuePair: {
        todo;
      }

      case NodeKind::Self: {
        if (!ctx.in_method) { throw err::semantics::cannot_use_self_here(node->token); }
        return make_class_type(ctx.cur_class->get_node());
      }

      case NodeKind::Array: {
        auto arr = node->as<NdArray>();

        std::vector<TypeInfo> elems;

        for (auto& elem : arr->data) {
          elems.push_back(eval_expr_ty(elem, ctx));
        }

        if (elems.empty()) {
          if (!ctx.empty_array_element_type) {
            todo; // cannot deduce element type of empty array
          }

          return TypeInfo(TypeKind::Vector, {*ctx.empty_array_element_type}, false, false);
        }

        for (size_t i = 1; i < elems.size(); i++) {
          if (!elems[i].equals(ctx.can_use_empty_array ? *ctx.empty_array_element_type
                                                       : elems[0])) {
            todo; // type mismatch
          }
        }

        return TypeInfo(TypeKind::Vector, std::move(elems), false, false);
      }

      case NodeKind::Tuple: {
        auto tuple = node->as<NdTuple>();

        std::vector<TypeInfo> elems;

        for (auto& elem : tuple->elems) {
          elems.push_back(eval_expr_ty(elem, ctx));
        }

        return TypeInfo(TypeKind::Tuple, std::move(elems), false, false);
      }

      case NodeKind::CallFunc: {
        auto cf = node->as<NdCallFunc>();

        // get argument types
        std::vector<TypeInfo> arg_types;
        for (auto& arg : cf->args) {
          arg_types.push_back(eval_expr_ty(arg, ctx));
        }

        // check argument matchings

        size_t argc_give = arg_types.size();

        NdEnumeratorDef* nd_en_def = nullptr;

        ctx.enumerator_node_out = &nd_en_def;

        auto callee_ty = eval_expr_ty(cf->callee, ctx); // callee_ty = { result_type, [args...] }

        if (callee_ty.is(TypeKind::Class)) { todo; }

        if (callee_ty.is(TypeKind::Enum)) {
          assert(nd_en_def);

          // no-variants
          if (nd_en_def->is_no_variants) {
            err::emitters::enumerator_no_have_variants(cf->args[0]->token, nd_en_def);
          }

          // one variant
          else if (nd_en_def->is_one_type) {
            if (argc_give == 0) {
              err::emitters::expected_one_variant_for_enumerator(cf->args[0]->token, nd_en_def);
            } else if (argc_give >= 2) {
              err::emitters::too_many_variants_for_enumerator(cf->args[0]->token, nd_en_def);
            } else if (argc_give == 1) {
              ctx = {};
              if (!arg_types[0].equals(eval_expr_ty(nd_en_def->variant_type, ctx))) {
                throw err::mismatched_types(
                    cf->args[0]->token, eval_typename_ty(nd_en_def->variant_type, ctx).to_string(),
                    arg_types[0].to_string());
              }
            }
          }

          // multiple variants or struct fields
          else if (nd_en_def->is_type_names || nd_en_def->is_struct_fields) {
            if (size_t count = nd_en_def->multiple.size(); count > argc_give) {
              err::emitters::too_few_variants_for_enumerator(cf->args[0]->token, nd_en_def);
            } else if (count < argc_give) {
              err::emitters::too_many_variants_for_enumerator(cf->args[0]->token, nd_en_def);
            } else {
              for (size_t i = 0; i < count; i++) {
                auto expected_ty =
                    nd_en_def->multiple[i]->is(NodeKind::Symbol)
                        ? eval_typename_ty(nd_en_def->multiple[i]->as<NdSymbol>(), ctx)
                        : eval_typename_ty(
                              nd_en_def->multiple[i]->as<NdKeyValuePair>()->value->as<NdSymbol>(),
                              ctx);

                if (!arg_types[i].equals(expected_ty)) {
                  throw err::mismatched_types(cf->args[i]->token, expected_ty.to_string(),
                                              arg_types[i].to_string());
                }
              }
            }
          }

          return callee_ty;
        }

        if (callee_ty.kind != TypeKind::Function) {
          todo; // callee must be function
        }

        size_t argc_take = callee_ty.parameters.size() - 1;

        if (argc_take != argc_give) {
          todo; // argument count mismatch
        }

        for (size_t i = 0; i < argc_take; i++) {
          if (!arg_types[i].equals(callee_ty.parameters[i + 1])) {
            todo; // argument type mismatch
          }
        }

        return callee_ty.parameters[0];
      }

      //
      // Subscript:
      case NodeKind::Subscript: {
        auto subs = node->as<NdExpr>();

        bool is_slice = subs->rhs->is(NodeKind::Slice);

        auto array_ty = eval_expr_ty(subs->lhs, ctx);

        if (is_slice) {
          auto slice = subs->rhs->as<NdExpr>();

          if (slice->lhs && !eval_expr_ty(slice->lhs, ctx).is(TypeKind::Int)) {
            todo; // index must be int
          }

          if (slice->rhs && !eval_expr_ty(slice->rhs, ctx).is(TypeKind::Int)) {
            todo; // end must be int
          }

          return array_ty;
        }

        auto index_ty = eval_expr_ty(subs->rhs, ctx);

        if (!index_ty.is(TypeKind::Int)) {
          todo; // index must be int
        }

        if (!array_ty.is(TypeKind::Vector)) {
          todo; // array must be vector
        }

        return array_ty.parameters[0];
      }

      case NodeKind::MemberAccess: {
        auto mm = node->as<NdExpr>();

        auto obj_ty = eval_expr_ty(mm->lhs, ctx);

        if (obj_ty.class_node == nullptr) {
          todo; // not instance.
        }

        todo;
      }

      case NodeKind::GetTupleElement: {
        todo;
      }

      case NodeKind::Inclement: {
        todo;
      }

      case NodeKind::Declement: {
        todo;
      }

      case NodeKind::BitNot: {
        todo;
      }

      case NodeKind::Not: {
        todo;
      }

      case NodeKind::Ref: {
        todo;
      }

      case NodeKind::Deref: {
        todo;
      }

      case NodeKind::Assign: {
        todo;
      }

      case NodeKind::AssignWithOp: {
        todo;
      }
    }

    assert(node->is_expr());

    auto ex = node->as<NdExpr>();

    auto lhs_ty = eval_expr_ty(ex->lhs, ctx);
    auto rhs_ty = eval_expr_ty(ex->rhs, ctx);

    // todo: check operators ...

    return lhs_ty;
  }

  TypeInfo TypeChecker::eval_typename_ty(NdSymbol* node, NdVisitorContext ctx) {
    (void)node;
    (void)ctx;

    switch (node->kind) {
      case NodeKind::Symbol: {
        auto sym = node->as<NdSymbol>();

        if (sym->symbol_ptr) {
          switch (sym->symbol_ptr->kind) {
            case SymbolKind::Enum:
              todo;

            case SymbolKind::Class:
              todo;

            case SymbolKind::BuiltinType: {
              TypeInfo ty = sym->symbol_ptr->type;

              for (auto p : sym->te_args) {
                ty.parameters.push_back(eval_expr_ty(p, ctx));
              }

              switch (ty.kind) {
                case TypeKind::Vector:
                  if (ty.parameters.size() != 1) {
                    todo; // vector must have one parameter
                  }
                  break;

                case TypeKind::Tuple:
                  if (ty.parameters.size() == 0) {
                    todo; // tuple must have parameters
                  }
                  break;

                case TypeKind::Dict:
                  if (ty.parameters.size() != 2) {
                    todo; // dict must have two parameters
                  }
                  break;
              }

              return ty;
            }

            case SymbolKind::TemplateParam:
              todo;
          }

          err::emitters::expected_type_name_here(node->token);
        }

        todo;
      }
    }

    todo;
  }

  TypeInfo TypeChecker::make_class_type(NdClass* node) {
    auto type = TypeInfo(TypeKind::Class);

    type.class_node = node;

    return type;
  }

  TypeInfo TypeChecker::make_enum_type(NdEnum* node) {
    auto type = TypeInfo(TypeKind::Enum);

    type.enum_node = node;

    debug(assert(node));

    return type;
  }

  void TypeChecker::check_expr(Node* node, NdVisitorContext ctx) {
  }

  void TypeChecker::check_stmt(Node* node, NdVisitorContext ctx) {
    switch (node->kind) {
      case NodeKind::Let: {
        auto let = node->as<NdLet>();

        if (let->type) {
          // auto type_ty = eval_typename_ty(let->type, ctx);
          // todo;
        }

        if (let->init) {
          auto init_ty = eval_expr_ty(let->init, ctx);
          (void)init_ty;
        }

        break;
      }
    }
  }

  void TypeChecker::check_scope(NdScope* node, NdVisitorContext ctx) {
  }

  void TypeChecker::check_function(NdFunction* node, NdVisitorContext ctx) {
  }

  void TypeChecker::check_class(NdClass* node, NdVisitorContext ctx) {
  }

  void TypeChecker::check_enum(NdEnum* node, NdVisitorContext ctx) {
  }

  void TypeChecker::check_namespace(NdNamespace* node, NdVisitorContext ctx) {
    ctx.cur_scope = node->scope_ptr->as<SCNamespace>();

    for (auto& item : node->items) {
      switch (item->kind) {
        case NodeKind::Let:
          check_stmt(item, ctx);
          break;
      }
    }
  }

  void TypeChecker::check_module(NdModule* node, NdVisitorContext ctx) {
    ctx.cur_scope = node->scope_ptr->as<SCModule>();

    for (auto& item : node->items) {
      switch (item->kind) {

        case NodeKind::Namespace: {
          check_namespace(item->as<NdNamespace>(), ctx);
          break;
        }

        case NodeKind::Let:
          check_stmt(item, ctx);
          break;
      }
    }
  }

} // namespace fire
