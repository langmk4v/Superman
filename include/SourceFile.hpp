#pragma once

#include <vector>
#include <string>

#include "FileSystem.hpp"

namespace fire {
  struct Token;
  struct NdModule;

  struct SourceFile {
    std::string path;
    std::string data;
    size_t length = 0;

    SourceFile* parent = nullptr;
    std::vector<SourceFile*> imports;

    bool is_node_imported = false;

    Token* lexed_token = nullptr;
    NdModule* parsed_mod = nullptr;

    SourceFile(std::string const& _path);

    Token* lex();

    NdModule* parse();

    SourceFile* import(std::string const& _path);

    void import_directory(std::string const& _path);

    size_t get_depth() const;

    std::string get_folder() const;

    char operator[](size_t const _index) const { return data[_index]; }
  };

} // namespace fire