#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fire {

  class FileSystem {
  public:
    struct DirectoryWrapper {
      std::string path;
      std::vector<DirectoryWrapper> directories;
      std::vector<std::filesystem::path> files;

      DirectoryWrapper(std::string const& path) : path(path) {
        if (!FileSystem::IsDirectory(path)) {
          throw std::invalid_argument("path is not a directory or not exists");
        }

        for (auto& entry : std::filesystem::directory_iterator(path)) {
          if (FileSystem::IsDirectory(entry.path().string())) {
            directories.push_back(DirectoryWrapper(entry.path().string()));
          } else if (FileSystem::IsFile(entry.path().string())) {
            files.push_back(entry.path());
          }
        }
      }

      DirectoryWrapper* FindDirectory(std::string const& path) {
        for (auto& dir : directories) {
          if (dir.path == path) {
            return &dir;
          }
        }
        return nullptr;
      }

      //
      // FindFile
      //   find all files that match the path
      //   if recursive is true, search recursively
      //   return the list of files
      //   if not found, return an empty list
      std::vector<std::string> FindFile(std::string const& path, bool recursive = false) {
        std::vector<std::string> hits;
        for (auto& file : files) {
          if (file.string() == path) {
            hits.push_back(file.string());
          }
        }
        if (recursive) {
          for (auto& dir : directories) {
            auto result = dir.FindFile(path, recursive);
            if (!result.empty()) {
              hits.insert(hits.end(), result.begin(), result.end());
            }
          }
        }
        return hits;
      }

      void Dump(int indent) const;
    };

    static std::string GetBaseName(std::string const& path) { return std::filesystem::path(path).filename().string(); }

    static bool Exists(std::string const& path) { return std::filesystem::exists(path); }

    static bool IsDirectory(std::string const& path) { return Exists(path) && std::filesystem::is_directory(path); }

    static bool IsFile(std::string const& path) { return Exists(path) && !IsDirectory(path); }

    static DirectoryWrapper GetDirectory(std::string const& path) { return DirectoryWrapper(path); }

    static std::vector<std::string> FindFileInDirectory(std::string const& filename, std::string const& directory,
                                                        bool recursive = false) {
      return GetDirectory(directory).FindFile(filename, recursive);
    }
  };

} // namespace fire