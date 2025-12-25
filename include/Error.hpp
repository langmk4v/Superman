#pragma once

#include "Utils.hpp"

#include "Lexer.hpp"
#include "Token.hpp"
#include "SourceFile.hpp"

namespace fire {
  enum errTypes {
    ET_Error,
    ET_Warn,
    ET_Note,
  };

  namespace err {
    struct e {
      SourceCode& s;
      size_t pos;
      size_t len;
      std::string msg;

      size_t line = 0;
      size_t column = 0;
      char const* tag = nullptr;
      bool is_warn = false;

      virtual e* print();

    protected:
      e(Token const& tok, std::string msg, errTypes et = ET_Error);
    };

    struct invalid_token : e {
      invalid_token(Token const& t) : e(t, "invalid token") {}
    };

    struct invalid_character_literal : e {
      invalid_character_literal(Token const& t) : e(t, "invalid character literal") {}
    };

    struct invalid_syntax : e {
      invalid_syntax(Token const& t) : e(t, "invalid syntax") {}
    };

    struct use_of_undefined_symbol : e {
      use_of_undefined_symbol(Token const& t) : e(t, "'" + t.text + "' is not found in this scope.") {}
    };

    struct not_callable_type : e {
      not_callable_type(Token const& t, std::string const& ti) : e(t, "'" + ti + "' type object is not callable") {}
    };

    struct expected_but_found : e {
      expected_but_found(Token const& t, char const* expected) : e(t, "expected '"s + expected + "'") {}
    };

    struct expected_identifier_tok : e {
      expected_identifier_tok(Token const& t) : e(t, "expected identifier") {}
    };

    struct scope_not_terminated : e {
      scope_not_terminated(Token const& t) : e(t, "scope not terminated") {}
    };

    struct unknown_type_name : e {
      unknown_type_name(Token const& t) : e(t, "unknown type name") {}
    };

    struct ambiguous_symbol_name : e {
      ambiguous_symbol_name(Token const& t) : e(t, "'" + t.text + "' is ambigous") {}
    };

    struct empty_class_or_enum_is_not_valid : e {
      empty_class_or_enum_is_not_valid(Token const& t) : e(t, "empty class or enum is not valid") {}
    };

    struct no_match_template_arguments : e {
      no_match_template_arguments(Token const& t, int Expected, int Found)
          : e(t, format("expected %d template parameters, but found %d.", Expected, Found)) {}
    };

    struct cannot_specify_return_type_of_constructor : e {
      cannot_specify_return_type_of_constructor(Token const& t) : e(t, "cannot_specify_return_type_of_constructor") {}
    };

    struct duplicate_of_definition : e {
      Token const& first;

      duplicate_of_definition(Token const& dup, Token const& first) : e(dup, "duplicate of definition"), first(first) {
        (void)first;
      }
    };

    struct mismatched_types : e {
      std::string expected;
      std::string found;

      mismatched_types(Token const& tok, std::string const& expected, std::string const& found)
          : e(tok, "expected '" + expected + "' type expression, but found '" + found + "'") {}
    };

    struct expected_item_of_module : e {
      expected_item_of_module(Token const& t)
          : e(t, "expected definition of function or class or enum after this token.") {}
    };

    struct mismatched_return_statement : e {
      mismatched_return_statement(Token const& t) : e(t, "mismatched return statement") {}
    };

    struct use_of_invalid_operator : e {
      use_of_invalid_operator(Token const& op, std::string const& left, std::string const& right)
          : e(op, "use of invalid operator '" + op.text + "' for types '" + left + "' and '" + right + "'") {}
    };

    struct too_few_arguments : e {
      too_few_arguments(Token const& t) : e(t, "too few arguments") {}
    };

    struct too_many_arguments : e {
      too_many_arguments(Token const& t) : e(t, "too many arguments") {}
    };

    namespace parses {
      struct expected_catch_block : e {
        expected_catch_block(Token const& t) : e(t, "expected catch block") {}
      };

      struct import_not_allowed_here : e {
        import_not_allowed_here(Token const& t) : e(t, "import is not allowed here") {}
      };

      struct cannot_use_decltype_here : e {
        cannot_use_decltype_here(Token const& t) : e(t, "cannot use decltype() here") {}
      };

      struct import_depth_limit_exceeded : e {
        import_depth_limit_exceeded(Token const& t, std::string const& path)
            : e(t, "import depth limit exceeded by importing the path '" + path + "'") {}
      };

      struct cannot_open_file : e {
        cannot_open_file(Token const& t, std::string const& path) : e(t, "cannot open file '" + path + "'") {}
      };
    } // namespace parses

    namespace semantics {
      struct this_is_not_typename : e {
        this_is_not_typename(Token const& t) : e(t, "'" + t.text + "' is not type name") {}
      };

      struct not_same_type_assignment : e {
        not_same_type_assignment(Token const& t, std::string const& dest, std::string const& src)
            : e(t, "cannot assignment to inequality type: '" + dest + "' <- '" + src + "'") {}
      };

      struct cannot_use_self_here : e {
        cannot_use_self_here(Token const& t) : e(t, "cannot use 'self' here.") {}
      };

      struct cannot_use_self_in_not_class_method : e {
        cannot_use_self_in_not_class_method(Token const& t)
            : e(t, "cannot use 'self' in a function not-method or static.") {}
      };

      struct cannot_use_typename_here : e {
        cannot_use_typename_here(Token const& t) : e(t, "cannot use type name here") {}
      };

      struct expected_expr_but_found : e {
        expected_expr_but_found(Token const& t, std::string const& other)
            : e(t, "expected an expression, but found " + other) {}
      };

      struct expected_class_type : e {
        expected_class_type(Token const& tok) : e(tok, "expected class type") {}
      };

      struct not_field_of_class : e {
        not_field_of_class(Token const& t, std::string const& Memb, std::string const& Clas)
            : e(t, "'" + Memb + "' is not field of class '" + Clas + "'") {}
      };
    } // namespace semantics

  } // namespace err

  namespace warns {
    struct W : err::e {
      W* operator()() {
        print();
        return this;
      }

    protected:
      using err::e::e;
    };

    struct added_pub_attr_automatically : W {
      added_pub_attr_automatically(Token const& t) : W(t, "visibility was changed to 'pub' automatically.", ET_Warn) {}
    };

    struct show_note : W {
      show_note(Token const& t, std::string const& msg) : W(t, msg, ET_Note) {}
    };
  } // namespace warns

} // namespace fire