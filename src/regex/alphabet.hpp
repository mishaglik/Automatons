#ifndef REGEX_ALPHABET_HPP
#define REGEX_ALPHABET_HPP

#include <cassert>
#include <cstdint>
namespace rgx {

template <std::size_t N>
  requires(N >= 0) && (N <= 26)
struct SimpleAlphabet {
  using CharT = char;
  static const constexpr uint64_t ErrorChr = ~0ul;
  static const constexpr CharT QuestionMark = '?';
  static const constexpr CharT Star = '*';
  static const constexpr CharT LBracket = '(';
  static const constexpr CharT RBracket = ')';
  static const constexpr CharT EmptyWord = '_';
  static const constexpr CharT Plus = '+';
  static const constexpr CharT EscapeChar = '\\';

  static constexpr bool IsSpace(CharT chr) { return chr == ' '; }

  static constexpr bool NeedEscape(CharT chr) {
    return chr == '(' || chr == ')' || chr == '\\' || chr == '*' ||
           chr == '_' || chr == '+' || chr == '?';
  }

  static const constexpr std::size_t Size = N + 1;

  static constexpr CharT Chr(uint64_t x) {
    assert(x < Size);
    if (x == 0) return 0;
    return 'a' + x - 1;
  }

  static constexpr uint64_t Ord(CharT chr) {
    if (chr == 0) return 0;
    if (static_cast<uint64_t>(chr - 'a') < N) return chr - 'a' + 1;
    return ErrorChr;
  }
};

struct CharAlphabet {
  using CharT = char;
  static const constexpr uint64_t ErrorChr = ~0ul;
  static const constexpr CharT QuestionMark = '?';
  static const constexpr CharT Star = '*';
  static const constexpr CharT LBracket = '(';
  static const constexpr CharT RBracket = ')';
  static const constexpr CharT EmptyWord = '_';
  static const constexpr CharT Plus = '+';
  static const constexpr CharT EscapeChar = '\\';

  static constexpr bool IsSpace(CharT chr) { return chr == ' '; }

  static constexpr bool NeedEscape(CharT chr) {
    return chr == '(' || chr == ')' || chr == '\\' || chr == '*' ||
           chr == '_' || chr == '+' || chr == '?';
  }

  static const constexpr std::size_t Size = 256;

  static constexpr CharT Chr(uint64_t x) { return static_cast<char>(x); }

  static constexpr uint64_t Ord(CharT chr) { return chr; }
};

struct AnyAlphabet {
  using CharT = uint64_t;
  static constexpr CharT Chr(uint64_t x) { return x; }
  static constexpr uint64_t Ord(CharT chr) { return chr; }
  static const constexpr CharT EscapeChar = ~0ul;
  static constexpr bool NeedEscape(CharT) {
    return false;
  }
};

}  // namespace rgx

#endif /* REGEX_ALPHABET_HPP */
