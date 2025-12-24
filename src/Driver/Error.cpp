#include "Utils/macro.h"
#include "Driver/Error.hpp"

namespace fire::err {

  e::e(Token const& tok, std::string msg, errTypes et)
      : s(tok.source), pos(tok.pos), len(tok.text.length()), msg(std::move(msg)), line(tok.line),
        column(tok.column) {
    if (et == ET_Error)
      tag = COL_RED "error" COL_DEFAULT;
    else if (et == ET_Warn)
      tag = COL_MAGENTA "warning" COL_DEFAULT;
    else if (et == ET_Note)
      tag = COL_CYAN "note" COL_DEFAULT;
    else
      tag = "???";
  }

  e* e::print() {
    size_t begin = 0, end = s.get_len();

    for (size_t i = 0; i <= pos; i++)
      if (s[i] == '\n')
        begin = i + 1;

    for (size_t i = pos; i < end; i++)
      if (s[i] == '\n') {
        end = i;
        break;
      }

    std::string linenum_s = std::to_string(line);

    std::cout
      << COL_BOLD << tag << ": " COL_WHITE << msg << COL_DEFAULT << COL_DEFAULT << std::endl
      << COL_LIGHT_GREEN << " -> " << s.path << ":" << line << ":" << column << COL_DEFAULT << std::endl
      << "  " << std::string(linenum_s.length(), ' ') << " |" << std::endl
      << "  " << linenum_s << " | " << s.data.substr(begin, end - begin) << std::endl
      << "  " << std::string(linenum_s.length(), ' ') << " |" << std::string(column, ' ') << "^" << std::endl
      << std::endl;

    return this;
  }

} // namespace superman::err