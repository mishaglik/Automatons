#ifndef REGEX_ALPHABET_HPP
#define REGEX_ALPHABET_HPP

#include <cassert>
#include <cstdint>
namespace rgx {

template <std::size_t N>
  requires(N >= 0) && (N <= 26)
struct SimpleAlphabet {
  using CharT = char;
  static const constexpr uint64_t kErrorChr = ~0ul;
  static const constexpr CharT kQuestionMark = '?';
  static const constexpr CharT kStar = '*';
  static const constexpr CharT kLBracket = '(';
  static const constexpr CharT kRBracket = ')';
  static const constexpr CharT kEmptyWord = '_';
  static const constexpr CharT kPlus = '+';
  static const constexpr CharT kConcat = '$';
  static const constexpr CharT kEscapeChar = '\\';

  static constexpr bool IsSpace(CharT chr) { return chr == ' '; }

  static constexpr bool NeedEscape(CharT chr) {
    return chr == '(' || chr == ')' || chr == '\\' || chr == '*' ||
           chr == '_' || chr == '+' || chr == '?' || chr == '$';
  }

  static const constexpr std::size_t kSize = N + 1;

  static constexpr CharT Chr(uint64_t x) {
    assert(x < kSize);
    if (x == 0) {
      return 0;
    }
    return 'a' + x - 1;
  }

  static constexpr uint64_t Ord(CharT chr) {
    if (chr == 0) {
      return 0;
    }
    if (static_cast<uint64_t>(chr - 'a') < N) {
      return chr - 'a' + 1;
    }
    return kErrorChr;
  }
};

template <std::size_t N>
  requires(N >= 0) && (N <= 26)
struct CanonicalAlphabet {
  using CharT = char;
  static const constexpr uint64_t kErrorChr = ~0ul;
  static const constexpr CharT kStar = '*';
  static const constexpr CharT kLBracket = '(';
  static const constexpr CharT kRBracket = ')';
  static const constexpr CharT kEmptyWord = '1';
  static const constexpr CharT kPlus = '+';
  static const constexpr CharT kConcat = '.';
  static const constexpr CharT kEscapeChar = '\\';
  static const constexpr CharT kQuestionMark = '?';

  static constexpr bool IsSpace(CharT chr) { return chr == ' '; }

  static constexpr bool NeedEscape(CharT chr) {
    return chr == '(' || chr == ')' || chr == '*' || chr == '_' || chr == '+' ||
           chr == '.';
  }

  static const constexpr std::size_t kSize = N + 1;

  static constexpr CharT Chr(uint64_t x) {
    assert(x < kSize);
    if (x == 0) {
      return 0;
    }
    return 'a' + x - 1;
  }

  static constexpr uint64_t Ord(CharT chr) {
    if (chr == 0) {
      return 0;
    }
    if (static_cast<uint64_t>(chr - 'a') < N) {
      return chr - 'a' + 1;
    }
    return kErrorChr;
  }
};

struct CharAlphabet {
  using CharT = char;
  static const constexpr uint64_t kErrorChr = ~0ul;
  static const constexpr CharT kQuestionMark = '?';
  static const constexpr CharT kStar = '*';
  static const constexpr CharT kLBracket = '(';
  static const constexpr CharT kRBracket = ')';
  static const constexpr CharT kEmptyWord = '_';
  static const constexpr CharT kPlus = '+';
  static const constexpr CharT kConcat = '$';
  static const constexpr CharT kEscapeChar = '\\';

  static constexpr bool IsSpace(CharT chr) { return chr == ' '; }

  static constexpr bool NeedEscape(CharT chr) {
    return chr == '(' || chr == ')' || chr == '\\' || chr == '*' ||
           chr == '_' || chr == '+' || chr == '?';
  }

  static const constexpr std::size_t kSize = 256;

  static constexpr CharT Chr(uint64_t x) { return static_cast<char>(x); }

  static constexpr uint64_t Ord(CharT chr) { return chr; }
};

struct AnyAlphabet {
  using CharT = uint64_t;
  static constexpr CharT Chr(uint64_t x) { return x; }
  static constexpr uint64_t Ord(CharT chr) { return chr; }
  static const constexpr CharT kEscapeChar = ~0ul;
  static constexpr bool NeedEscape(CharT) { return false; }
};

}  // namespace rgx

#endif /* REGEX_ALPHABET_HPP */
