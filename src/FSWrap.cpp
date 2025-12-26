#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <cstring>

#include "FileSystem.hpp"

namespace fire {

  void FileSystem::SetCwd(std::string const& path) {
    if(chdir(path.c_str()) != 0){
      throw std::runtime_error("failed to set cwd: " + path);
    }
  }

  std::string FileSystem::GetCwd() {
    char buf[PATH_MAX];
    return getcwd(buf, sizeof(buf)) ? buf : "";
  }

  std::string FileSystem::GetBaseName(std::string const& path) {
    auto pos = path.find_last_of('/');
    return (pos == std::string::npos) ? path : path.substr(pos + 1);
  }

  std::string FileSystem::GetFolderOfFile(std::string const& path) {
    auto pos = path.find_last_of('/');
    return (pos == std::string::npos) ? "" : path.substr(0, pos);
  }

  bool FileSystem::Exists(std::string const& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
  }

  bool FileSystem::IsDirectory(std::string const& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
  }

  bool FileSystem::IsFile(std::string const& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
  }

} // namespace fire
