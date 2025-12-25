#pragma once

#include <vector>
#include <string>

#include "SourceFile.hpp"
#include "Token.hpp"

namespace fire {
  class Lexer {
    SourceCode& _source;
    size_t _pos;
    size_t const _len;

  public:
    Lexer(SourceCode& source) : _source(source), _pos(0), _len(source.get_len()) {}

    std::vector<Token> lex();

    //
    // - remove_all_comments
    // すべてのコメントを空白に置き換え
    // ( エラー表示時に行・列の番号が変わってしまうため、削除はしない )
    void remove_all_comments();

    int find_comment(int begin) {
      for (int i = begin; i + 2 < static_cast<int>(_len); i++)
        if (match("//") || match("/*")) return i;

      return -1;
    }

  private:
    bool is_end() { return _pos >= _len; }

    char peek() { return _source[_pos]; }

    void pass_space() {
      while (!is_end() && isspace(peek()))
        _pos++;
    }

    bool match(std::string const& s) { return _pos + s.length() <= _len && _source.data.substr(_pos, s.length()) == s; }

    //
    // - erase
    //
    // 現在位置から文字を n 個削除する・または c に置き換える
    // 終端までに n 個分の文字がない場合は何もせず false を返す
    bool erase(int n, char c = -1) { // -1 = 削除
      if (_pos + n > _len) return false;

      if (c == -1)
        _source.data.erase(_source.data.begin() + _pos, _source.data.begin() + _pos + n);
      else
        std::fill(_source.data.begin() + _pos, _source.data.begin() + _pos + n, c);

      return true;
    }

    inline void replace(int n, char c) { erase(n, c); }

    Token tokenize(char c);
  };
} // namespace fire