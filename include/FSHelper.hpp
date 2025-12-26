#pragma once

#include <string>
#include <vector>
#include <stdexcept>

#include "fs_impl.hpp"

namespace fire {

  class FileSystem {
  public:
    class DirectoryWrapper {
    public:
      explicit DirectoryWrapper(std::string const& path) : impl(fs_dir_create(path.c_str())) {
        if (!impl) {
          throw std::invalid_argument("path is not a directory or not exists");
        }
      }

      DirectoryWrapper(DirectoryWrapper const&) = delete;
      DirectoryWrapper& operator=(DirectoryWrapper const&) = delete;

      DirectoryWrapper(DirectoryWrapper&& other) noexcept : impl(other.impl) { other.impl = nullptr; }

      DirectoryWrapper& operator=(DirectoryWrapper&& other) noexcept {
        if (this != &other) {
          fs_dir_destroy(impl);
          impl = other.impl;
          other.impl = nullptr;
        }
        return *this;
      }

      ~DirectoryWrapper() { fs_dir_destroy(impl); }

      // -----------------------------------------
      // 検索系
      // -----------------------------------------
      std::vector<std::string> FindFile(std::string const& filename, bool recursive = false) const {
        char** results = nullptr;
        size_t count = 0;

        fs_dir_find_file(impl, filename.c_str(), recursive ? 1 : 0, &results, &count);

        std::vector<std::string> out;
        out.reserve(count);

        for (size_t i = 0; i < count; ++i) {
          out.emplace_back(results[i]);
          free(results[i]);
        }
        free(results);

        return out;
      }

      bool Contains_File(std::string const& filename) const {
        return fs_dir_contains_file(impl, filename.c_str()) != 0;
      }

      bool Contains_Directory(std::string const& dirname) const {
        return fs_dir_contains_dir(impl, dirname.c_str()) != 0;
      }

      // -----------------------------------------
      // Dump（C 実装）
      // -----------------------------------------
      void Dump(int indent = 0) const { fs_dir_dump(impl, indent); }

    private:
      fs_dir* impl = nullptr;
    };

    // -----------------------------------------
    // FileSystem API
    // -----------------------------------------
    static void SetCwd(std::string const& path);
    static std::string GetCwd();

    static std::string GetBaseName(std::string const& path);
    static std::string GetFolderOfFile(std::string const& path);

    static bool Exists(std::string const& path);
    static bool IsDirectory(std::string const& path);
    static bool IsFile(std::string const& path);

    static DirectoryWrapper GetDirectory(std::string const& path) { return DirectoryWrapper(path); }

    static std::vector<std::string> FindFileInDirectory(std::string const& filename, std::string const& directory,
                                                        bool recursive = false) {
      return DirectoryWrapper(directory).FindFile(filename, recursive);
    }
  };

} // namespace fire
