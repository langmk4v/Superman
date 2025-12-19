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

      virtual e* print() {
        size_t begin = 0, end = s.get_len();
        for (size_t i = pos; i > 0; i--)
          if (s[i] == '\n') {
            begin = i + 1;
            break;
          }
        for (size_t i = pos; i < end; i++)
          if (s[i] == '\n') {
            end = i;
            break;
          }
        printf("%s: %s\n -> %s:%zu:%zu\n  % 3d | %s\n      |%s^\n\n", tag, msg.c_str(),
               s.path.c_str(), line, column, (int)line, s.data.substr(begin, end - begin).c_str(),
               string(column, ' ').c_str());
        return this;
      }

    protected:
      e(Token const& tok, std::string msg, errTypes et = ET_Error)
          : s(tok.source), pos(tok.pos), len(tok.text.length()), msg(std::move(msg)),
            line(tok.line), column(tok.column) {
        if (et == ET_Error)
          tag = COL_RED "error" COL_DEFAULT;
        else if (et == ET_Warn)
          tag = COL_MAGENTA "warning" COL_DEFAULT;
        else if (et == ET_Note)
          tag = COL_CYAN "note" COL_DEFAULT;
        else
          tag = "???";
      }
    };

    struct invalid_token : e {
      invalid_token(Token const& t) : e(t, "invalid token") {}
    };

    struct invalid_syntax : e {
      invalid_syntax(Token const& t) : e(t, "invalid syntax") {}
    };

    struct expected_but_found : e {
      expected_but_found(Token const& t, char const* expected)
          : e(t, "expected '"s + expected + "'") {}
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
      ambiguous_symbol_name(Token const& t) : e(t, "ambiguous_symbol_name") {}
    };

    struct empty_class_or_enum_is_not_valid : e {
      empty_class_or_enum_is_not_valid(Token const& t) : e(t, "empty class or enum is not valid") {}
    };

    struct no_match_template_arguments : e {
      no_match_template_arguments(Token const& t, int Expected, int Found)
          : e(t,
              strings::format("expected %d template parameters, but found %d.", Expected, Found)) {}
    };

    struct cannot_specify_return_type_of_constructor : e {
      cannot_specify_return_type_of_constructor(Token const& t)
          : e(t, "cannot_specify_return_type_of_constructor") {}
    };

    struct duplicate_of_definition : e {
      Token const& first;

      duplicate_of_definition(Token const& err, Token const& first)
          : e(err, "duplicate of definition"), first(first) {
        (void)first;
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
          : W(t, "visibility was changed to 'pub' automatically.", ET_Warn) {}
    };

    struct show_note : W {
      show_note(Token const& t, std::string const& msg) : W(t, msg, ET_Note) {}
    };
  } // namespace warns

} // namespace superman