#include <iostream>
#include "FSWrap.hpp"

namespace fire {

  void FileSystem::DirectoryWrapper::Dump(int indent) const {
    std::string ind(indent, ' ');
    std::cout << ind << FileSystem::GetBaseName(path) << "/" << std::endl;
    for (auto& dir : directories) {
      dir.Dump(indent + 2);
    }
    for (auto& file : files) {
      std::cout << ind << "  " << FileSystem::GetBaseName(file.string()) << std::endl;
    }
  }

} // namespace fire