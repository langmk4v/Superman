#include <fstream>
#include <filesystem>
#include <unordered_map>

#include "Utils.hpp"
#include "Token.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include "SourceFile.hpp"

namespace fire {
  std::unordered_map<std::string, SourceFile*> all_sources;

  SourceFile::SourceFile(std::string const& _path) : path(std::filesystem::absolute(_path)) {
    auto ifs = std::ifstream(this->path);

    if (ifs.fail()) {
      std::cout << "cannot open file: " << this->path << std::endl;
      std::exit(1);
    }

    for (std::string line; std::getline(ifs, line);)
      this->data.append(line.append("\n"));

    all_sources[this->path] = this;
  }

  NdModule* SourceFile::parse() {
    this->lexed_token = Lexer(this).lex();

    this->parsed_mod = Parser(*this, this->lexed_token).parse();

    return this->parsed_mod;
  }

  //
  // import a file
  SourceFile* SourceFile::import(std::string const& _path) {
    if (all_sources.find(_path) != all_sources.end()) { return all_sources[_path]; }

    auto new_source = new SourceFile(_path);

    new_source->parent = this;

    this->imports.push_back(new_source);

    return new_source;
  }

  //
  // import a directory
  void SourceFile::import_directory(std::string const& _path) {
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

  std::string SourceFile::get_folder() const {
    return path.substr(0, path.find_last_of('/') + 1);
  }

  size_t SourceFile::get_depth() const {
    return parent ? parent->get_depth() + 1 : 0;
  }

} // namespace fire