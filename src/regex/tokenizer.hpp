#ifndef REGEX_TOKENIZER_HPP
#define REGEX_TOKENIZER_HPP

#include <cctype>
#include <cstdint>
#include <iostream>
#include <string_view>
namespace rgx {

template <typename Alphabet>
struct RegexToken {
  using CharT = typename Alphabet::CharT;
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
    if (type != oth.type) {
      return false;
    }
    return (type != Type::Letter) || chr == oth.chr;
  }

  bool operator!=(const RegexToken& oth) const = default;

  explicit operator bool() const { return type != Type::Error; }
};

template <typename Alphabet>
class Tokenizer;

template <typename Alphabet>
class TokenIterator {
 public:
  using CharT = typename Alphabet::CharT;
  using Token = RegexToken<Alphabet>;

 private:
  friend class Tokenizer<Alphabet>;
  std::basic_string_view<CharT> str_;
  std::size_t pos_ = 0;
  Token token_ = {Token::Type::EOL};

  explicit TokenIterator(std::basic_string_view<CharT> str, bool end = false)
      : str_(str) {
    if (end) {
      pos_ = str_.length();
    }
    GetNextToken();
  }

  void SkipSpaces() {
    while (pos_ < str_.length() && Alphabet::IsSpace(str_[pos_])) {
      pos_++;
    }
  }

  void GetNextToken() {
    SkipSpaces();

    if (pos_ >= str_.length()) {
      token_ = RegexToken<Alphabet>{Token::Type::EOL};
      return;
    }

    if (str_[pos_] == Alphabet::kEscapeChar) {
      if (pos_ + 1 >= str_.length()) {
        token_.type = Token::Type::Error;
        return;
      }

      uint64_t chr = Alphabet::Ord(str_[++pos_]);
      if (chr == Alphabet::kErrorChr) {
        token_ = {Token::Type::Error};

      } else {
        token_ = {Token::Type::Letter, chr};
      }
      pos_++;
      return;
    }

    if (str_[pos_] == Alphabet::kStar) {
      token_.type = Token::Type::KleeneStar;
      pos_++;
      return;
    }

    if (str_[pos_] == Alphabet::kQuestionMark) {
      token_.type = Token::Type::QuestionMark;
      pos_++;
      return;
    }

    if (str_[pos_] == Alphabet::kPlus) {
      token_.type = Token::Type::Alternate;
      pos_++;
      return;
    }

    if (str_[pos_] == Alphabet::kLBracket) {
      token_.type = Token::Type::LBracket;
      pos_++;
      return;
    }

    if (str_[pos_] == Alphabet::kRBracket) {
      token_.type = Token::Type::RBracket;
      pos_++;
      return;
    }

    if (str_[pos_] == Alphabet::kEmptyWord) {
      token_.type = Token::Type::Empty;
      pos_++;
      return;
    }

    uint64_t chr = Alphabet::Ord(str_[pos_++]);
    if (chr == Alphabet::kErrorChr) {
      token_ = {Token::Type::Error};

    } else {
      token_ = {Token::Type::Letter, chr};
    }
  }

 public:
  TokenIterator(const TokenIterator&) = default;
  TokenIterator& operator=(const TokenIterator&) = default;

  const Token& operator*() const { return token_; }
  const Token* operator->() const { return &token_; }

  TokenIterator& operator++() {
    GetNextToken();
    return *this;
  }

  TokenIterator operator++(int) {
    TokenIterator copy = *this;
    GetNextToken();
    return copy;
  }

  bool operator==(const TokenIterator& oth) const {
    assert(str_.data() == oth.str_.data());
    return pos_ == oth.pos_;
  }

  bool operator!=(const TokenIterator& oth) const = default;
};

template <typename Alphabet>
class Tokenizer {
  using CharT = typename Alphabet::CharT;
  std::basic_string_view<CharT> str_;

 public:
  explicit Tokenizer(std::basic_string_view<CharT> str) : str_(str) {}
  // NOLINTBEGIN
  TokenIterator<Alphabet> begin() const {
    return TokenIterator<Alphabet>(str_, false);
  }

  TokenIterator<Alphabet> end() const {
    return TokenIterator<Alphabet>(str_, true);
  }
  // NOLINTEND
};

}  // namespace rgx

#endif /* REGEX_TOKENIZER_HPP */
