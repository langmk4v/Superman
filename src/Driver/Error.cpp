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
    printf("%s: %s\n -> %s:%zu:%zu\n  % 3d | %s\n      |%s^\n\n", tag, msg.c_str(), s.path.c_str(),
           line, column, (int)line, s.data.substr(begin, end - begin).c_str(),
           string(column, ' ').c_str());
    return this;
  }

} // namespace superman::err