#pragma once

#include "FileSystem.hpp"

namespace fire {
  struct Token;
  struct NdModule;

  struct SourceFile {
    char const* path = nullptr;

    char const* data = nullptr;
    size_t const length;

    SourceFile(std::string const& _path);

    NdModule* parse();

    SourceFile* import(std::string const& _path);

    void import_directory(std::string const& _path);

    size_t get_depth() const;

    std::string_view get_folder() const;

    char operator[](size_t const _index) const { return data[_index]; }
  };

} // namespace fire