#pragma once

#include <stddef.h>

typedef struct fs_file {
  char* path;
} fs_file;

typedef struct fs_dir {
  char* name;
  char* full_path;

  struct fs_dir** dirs;
  size_t dir_count;

  fs_file* files;
  size_t file_count;
} fs_dir;

/* 作成・破棄 */
fs_dir* fs_dir_create(char const* path);
void fs_dir_destroy(fs_dir* dir);

/* 検索 */
int fs_dir_contains_file(fs_dir const* dir, char const* filename);
int fs_dir_contains_dir(fs_dir const* dir, char const* dirname);

void fs_dir_find_file(fs_dir const* dir, char const* filename, int recursive, char*** results, size_t* count);

/* Dump */
void fs_dir_dump(fs_dir const* dir, int indent);
