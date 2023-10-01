#ifndef REGEX_TOKENIZER_HPP
#define REGEX_TOKENIZER_HPP

#include <cctype>
#include <cstdint>
#include <iostream>
#include <string_view>
namespace rgx {

template <typename Alphabet>
struct RegexToken {
  using CharT = Alphabet::CharT;
  enum class Type {
    Error = 0,
    EOL,
    Letter,
    KleeneStar,
    QuestionMark,
    Alternate,
    LBracket,
    RBracket,
    Empty,
  };

  Type type = Type::Error;
  uint64_t chr = 0;  // Valid only when type == Letter

  bool operator==(const RegexToken& oth) const {
    if (type != oth.type) return false;
    return (type != Type::Letter) || chr == oth.chr;
  }

  bool operator!=(const RegexToken& oth) const = default;

  operator bool() const { return type != Type::Error; }
};

template <typename Alphabet>
class Tokenizer;

template <typename Alphabet>
class TokenIterator {
 public:
  using CharT = Alphabet::CharT;
  using Token = RegexToken<Alphabet>;

 private:
  friend class Tokenizer<Alphabet>;
  std::basic_string_view<CharT> m_str;
  std::size_t m_pos = 0;
  Token m_token = {Token::Type::EOL};

  TokenIterator(std::basic_string_view<CharT> str, bool end = false)
      : m_str(str) {
    if (end) {
      m_pos = m_str.length();
    }
    getNextToken();
  }

  void skipSpaces() {
    while (m_pos < m_str.length() && Alphabet::IsSpace(m_str[m_pos])) {
      m_pos++;
    }
  }

  void getNextToken() {
    skipSpaces();

    if (m_pos >= m_str.length()) {
      m_token = RegexToken<Alphabet>{Token::Type::EOL};
      return;
    }

    if (m_str[m_pos] == Alphabet::EscapeChar) {
      if (m_pos + 1 >= m_str.length()) {
        m_token.type = Token::Type::Error;
        return;
      }

      uint64_t chr = Alphabet::Ord(m_str[++m_pos]);
      if (chr == Alphabet::ErrorChr) {
        m_token = {Token::Type::Error};

      } else {
        m_token = {Token::Type::Letter, chr};
      }
      m_pos++;
      return;
    }

    if (m_str[m_pos] == Alphabet::Star) {
      m_token.type = Token::Type::KleeneStar;
      m_pos++;
      return;
    }

    if (m_str[m_pos] == Alphabet::QuestionMark) {
      m_token.type = Token::Type::QuestionMark;
      m_pos++;
      return;
    }

    if (m_str[m_pos] == Alphabet::Plus) {
      m_token.type = Token::Type::Alternate;
      m_pos++;
      return;
    }

    if (m_str[m_pos] == Alphabet::LBracket) {
      m_token.type = Token::Type::LBracket;
      m_pos++;
      return;
    }

    if (m_str[m_pos] == Alphabet::RBracket) {
      m_token.type = Token::Type::RBracket;
      m_pos++;
      return;
    }

    if (m_str[m_pos] == Alphabet::EmptyWord) {
      m_token.type = Token::Type::Empty;
      m_pos++;
      return;
    }

    uint64_t chr = Alphabet::Ord(m_str[m_pos++]);
    if (chr == Alphabet::ErrorChr) {
      m_token = {Token::Type::Error};

    } else {
      m_token = {Token::Type::Letter, chr};
    }
    return;
  }

 public:
  TokenIterator(const TokenIterator&) = default;
  TokenIterator& operator=(const TokenIterator&) = default;

  const Token& operator*() const { return m_token; }
  const Token* operator->() const { return &m_token; }

  TokenIterator& operator++() {
    getNextToken();
    return *this;
  }

  TokenIterator operator++(int) {
    TokenIterator copy = *this;
    getNextToken();
    return copy;
  }

  bool operator==(const TokenIterator& oth) const {
    assert(m_str.data() == oth.m_str.data());
    return m_pos == oth.m_pos;
  }

  bool operator!=(const TokenIterator& oth) const = default;
};

template <typename Alphabet>
class Tokenizer {
  using CharT = Alphabet::CharT;
  std::basic_string_view<CharT> m_str;

 public:
  Tokenizer(std::basic_string_view<CharT> str) : m_str(str) {}

  TokenIterator<Alphabet> begin() const {
    return TokenIterator<Alphabet>(m_str, false);
  }

  TokenIterator<Alphabet> end() const {
    return TokenIterator<Alphabet>(m_str, true);
  }
};

}  // namespace rgx

#endif /* REGEX_TOKENIZER_HPP */
