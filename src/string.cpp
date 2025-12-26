#include <cstring>
#include "string.hpp"

#define FIRST_BUFFER_SIZE 0x100

namespace fire {
  void string::_check_buf_size(size_t want_add_size) {
    if (_length + want_add_size < _buf_size)
      return;

    while (_length + want_add_size >= _buf_size)
      _buf_size *= 2;

    auto newbuf = new char[_buf_size];
    std::memcpy(newbuf, _buffer, _length);
    delete[] _buffer;
    _buffer = newbuf;
  }

  string::string() : _buffer(new char[FIRST_BUFFER_SIZE]), _length(0), _buf_size(FIRST_BUFFER_SIZE) {
  }

  string::string(string&& other) : _buffer(other._buffer), _length(other._length), _buf_size(other._buf_size) {
    other._buffer = nullptr;
    other._length = 0;
    other._buf_size = 0;
  }

  string::~string() {
    if (_buffer) {
      delete[] _buffer;
    }
  }

  size_t string::length() const {
    return _length;
  }

  char* string::raw() {
    return _buffer;
  }

  char const* string::raw() const {
    return _buffer;
  }

  bool string::empty() const {
    return _length == 0;
  }

  bool string::equals(string const& s) const {
    return _length == s._length && std::memcmp(_buffer, s._buffer, _length) == 0;
  }
  
  bool string::contains(char c) const {
    return std::strchr(_buffer, c) != nullptr;
  }

  bool string::contains(string const& s) const {
    return std::strstr(_buffer, s._buffer) != nullptr;
  }

  string& string::append(char c) {
    _check_buf_size(1);
    _buffer[_length] = c;
    _length++;
    return *this;
  }

  string& string::append_str(string const& s) {
    _check_buf_size(s._length);
    std::memcpy(_buffer + _length, s._buffer, s._length);
    _length += s._length;
    return *this;
  }

  string& string::append_str_ptr(char const* p) {
    _check_buf_size(std::strlen(p));
    std::memcpy(_buffer + _length, p, std::strlen(p));
    _length += std::strlen(p);
    return *this;
  }

  string string::clone() const {
    string s;
    s._buffer = new char[_length];
    std::memcpy(s._buffer, _buffer, _length);
    s._length = _length;
    s._buf_size = _buf_size;
    return s;
  }

  string string::from_char(char c) {
    string s;
    s.append(c);
    return s;
  }

  string string::from_pointer(char const* p) {
    string s;
    s.append_str_ptr(p);
    return s;
  }

  string string::from_pointer_move(char* p) {
    string s;
    s._buffer = p;
    s._length = std::strlen(p);
    s._buf_size = s._length;
    return s;
  }
}