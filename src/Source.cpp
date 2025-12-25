#include <fstream>
#include <filesystem>
#include <unordered_map>

#include "Utils.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include "SourceFile.hpp"

namespace fire {
  std::unordered_map<std::string, SourceCode*> all_sources;

  SourceCode::SourceCode(std::string const& _path) : path(std::filesystem::absolute(_path)) {
    auto ifs = std::ifstream(this->path);

    if (ifs.fail()) throw new std::invalid_argument("cannot open file");

    for (std::string line; std::getline(ifs, line);)
      this->data.append(line.append("\n"));

    all_sources[this->path] = this;
  }

  NdModule* SourceCode::parse() {
    this->tokens = Lexer(*this).lex();

    this->parsed_mod = Parser(*this, this->tokens).parse();

    return this->parsed_mod;
  }

  //
  // import a file
  SourceCode* SourceCode::import(std::string const& _path) {
    if (all_sources.find(_path) != all_sources.end()) {
      return all_sources[_path];
    }

    auto new_source = new SourceCode(_path);

    new_source->parent = this;

    this->imports.push_back(new_source);

    return new_source;
  }

  //
  // import a directory
  void SourceCode::import_directory(std::string const& _path) {
    //
    // get all files in the directory
    for (auto& entry : std::filesystem::directory_iterator(_path)) {
      if (entry.is_directory()) {
        this->import_directory(entry.path().string());
      } else {
        this->import(entry.path().string());
      }
    }
  }

  std::string SourceCode::get_folder() const { return path.substr(0, path.find_last_of('/') + 1); }

  size_t SourceCode::get_depth() const { return parent ? parent->get_depth() + 1 : 0; }

} // namespace fire