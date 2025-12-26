#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs_impl.hpp"

static int is_directory(char const* path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int is_file(char const* path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static char* dup_basename(char const* path) {
  char const* p = strrchr(path, '/');
  return strdup(p ? p + 1 : path);
}

fs_dir* fs_dir_create(char const* path) {
  if (!is_directory(path)) return NULL;

  fs_dir* dir = (fs_dir*)calloc(1, sizeof(fs_dir));
  dir->full_path = strdup(path);
  dir->name = dup_basename(path);

  DIR* d = opendir(path);
  struct dirent* ent;

  while ((ent = readdir(d))) {
    if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) continue;

    char full[4096];
    snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);

    if (is_directory(full)) {
      dir->dirs = (fs_dir**)realloc(dir->dirs, sizeof(fs_dir*) * (dir->dir_count + 1));
      dir->dirs[dir->dir_count++] = fs_dir_create(full);
    } else if (is_file(full)) {
      dir->files = (fs_file*)realloc(dir->files, sizeof(fs_file) * (dir->file_count + 1));
      dir->files[dir->file_count++].path = strdup(full);
    }
  }
  closedir(d);
  return dir;
}

void fs_dir_destroy(fs_dir* dir) {
  if (!dir) return;
  for (size_t i = 0; i < dir->dir_count; ++i)
    fs_dir_destroy(dir->dirs[i]);
  for (size_t i = 0; i < dir->file_count; ++i)
    free(dir->files[i].path);

  free(dir->dirs);
  free(dir->files);
  free(dir->name);
  free(dir->full_path);
  free(dir);
}

void fs_dir_find_file(fs_dir const* dir, char const* filename, int recursive, char*** results, size_t* count) {
  for (size_t i = 0; i < dir->file_count; ++i) {
    if (!strcmp(strrchr(dir->files[i].path, '/') + 1, filename)) {
      *results = (char**)realloc(*results, sizeof(char*) * (*count + 1));
      (*results)[(*count)++] = strdup(dir->files[i].path);
    }
  }
  if (!recursive) return;

  for (size_t i = 0; i < dir->dir_count; ++i) {
    fs_dir_find_file(dir->dirs[i], filename, recursive, results, count);
  }
}

void fs_dir_dump(fs_dir const* dir, int indent) {
  for (int i = 0; i < indent; ++i)
    putchar(' ');
  printf("%s/\n", dir->name);

  for (size_t i = 0; i < dir->dir_count; ++i)
    fs_dir_dump(dir->dirs[i], indent + 2);

  for (size_t i = 0; i < dir->file_count; ++i) {
    for (int j = 0; j < indent + 2; ++j)
      putchar(' ');
    printf("%s\n", strrchr(dir->files[i].path, '/') + 1);
  }
}
