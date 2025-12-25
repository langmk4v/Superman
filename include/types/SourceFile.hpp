#pragma once

#include <string>
#include <vector>

namespace fire {
  struct Token;
  struct NdModule;

  struct SourceCode {
    std::string path;
    std::string data;

    std::vector<Token> tokens;

    NdModule* parsed_mod = nullptr;

    SourceCode* parent = nullptr;
    std::vector<SourceCode*> imports;

    bool is_node_imported = false;

    SourceCode(std::string const& _path);

    NdModule* parse();

    SourceCode* import(std::string const& _path);

    void import_directory(std::string const& _path);

    size_t get_depth() const;

    std::string get_folder() const;

    size_t get_len() const { return data.length(); }

    char& operator[](size_t const _index) { return data[_index]; }

    char operator[](size_t const _index) const { return data[_index]; }
  };

} // namespace fire