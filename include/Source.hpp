#pragma once

#include <string>
#include <fstream>
#include <filesystem>

namespace superman {
  struct SourceCode {
    std::string path;
    std::string data;

    SourceCode(std::string const& _path);

    size_t get_len() const { return data.length(); }

    char& operator[](size_t const _index) { return data[_index]; }

    char operator[](size_t const _index) const { return data[_index]; }
  };
} // namespace superman