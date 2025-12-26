#pragma once

#include "defs.hpp"

namespace fire {
  class string {
    char* _buffer;
    size_t _length;
    size_t _buf_size;

    void _check_buf_size(int want_add_size);

  public:
    string();
    string(string&&);
    string(string const&) = delete;
    ~string();

    size_t length() const;
    char* raw();
    char const* raw() const;

    bool empty() const;
    bool equals(string const& s) const;
    
    bool contains(char c) const;
    bool contains(string const& s) const;

    string& append(char c);
    string& append_str(string const& s);
    string& append_str_ptr(char const* p);

    string clone() const;

    static string from_char(char c);
    static string from_pointer(char const* p);
    static string from_pointer_move(char* p); // don't copy
  };
}