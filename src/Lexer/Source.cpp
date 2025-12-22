#include <fstream>
#include <filesystem>

#include "Lexer/Source.hpp"

namespace fire::lexer {

  SourceCode::SourceCode(std::string const& _path) : path(std::filesystem::absolute(_path)) {
    auto ifs = std::ifstream(this->path);

    if (ifs.fail()) throw new std::invalid_argument("cannot open file");

    for (std::string line; std::getline(ifs, line);)
      this->data.append(line.append("\n"));
  }

} // namespace superman