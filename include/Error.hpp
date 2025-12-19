#pragma once

#include "Strings.hpp"
#include "Token.hpp"
#include "Source.hpp"

namespace superman {
  using std::string;
  using std::string_literals::operator""s;

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
      invalid_token(Token const& t) : e(t, "invalid token") {
      }
    };

    struct invalid_syntax : e {
      invalid_syntax(Token const& t) : e(t, "invalid syntax") {
      }
    };

    struct expected_but_found : e {
      expected_but_found(Token const& t, char const* expected)
          : e(t, "expected '"s + expected + "'") {
      }
    };

    struct expected_identifier_tok : e {
      expected_identifier_tok(Token const& t) : e(t, "expected identifier") {
      }
    };

    struct scope_not_terminated : e {
      scope_not_terminated(Token const& t) : e(t, "scope not terminated") {
      }
    };

    struct unknown_type_name : e {
      unknown_type_name(Token const& t) : e(t, "unknown type name") {
      }
    };

    struct ambiguous_symbol_name : e {
      ambiguous_symbol_name(Token const& t) : e(t, "ambiguous_symbol_name") {
      }
    };

    struct empty_class_or_enum_is_not_valid : e {
      empty_class_or_enum_is_not_valid(Token const& t) : e(t, "empty class or enum is not valid") {
      }
    };

    struct no_match_template_arguments : e {
      no_match_template_arguments(Token const& t, int Expected, int Found)
          : e(t,
              strings::format("expected %d template parameters, but found %d.", Expected, Found)) {
      }
    };

    struct cannot_specify_return_type_of_constructor : e {
      cannot_specify_return_type_of_constructor(Token const& t)
          : e(t, "cannot_specify_return_type_of_constructor") {
      }
    };

    struct duplicate_of_definition : e {
      Token const& first;

      duplicate_of_definition(Token const& dup, Token const& first)
          : e(dup, "duplicate of definition"), first(first) {
        (void)first;
      }
    };

    struct mismatched_types : e {
      std::string expected;
      std::string found;

      mismatched_types(Token const& tok, std::string const& expected, std::string const& found)
          : e(tok, "expected '" + expected + "' type, but found '" + found + "'") {
      }
    };

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
      added_pub_attr_automatically(Token const& t)
          : W(t, "visibility was changed to 'pub' automatically.", ET_Warn) {
      }
    };

    struct show_note : W {
      show_note(Token const& t, std::string const& msg) : W(t, msg, ET_Note) {
      }
    };
  } // namespace warns

} // namespace superman