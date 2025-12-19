#pragma once

#include <string>
#include <fstream>
#include <filesystem>

namespace superman {
  struct SourceCode {
    std::string path;
    std::string data;

    SourceCode(std::string const& _path) : path(std::filesystem::absolute(_path)) {
      auto ifs = std::ifstream(this->path);

      if (ifs.fail()) throw new std::invalid_argument("cannot open file");

      for (std::string line; std::getline(ifs, line);)
        this->data.append(line.append("\n"));
    }

    size_t get_len() const { return data.length(); }

    char& operator[](size_t const _index) { return data[_index]; }

    char operator[](size_t const _index) const { return data[_index]; }
  };
} // namespace superman