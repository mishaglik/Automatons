#ifndef REGEX_REGEX_HPP
#define REGEX_REGEX_HPP

#include <string>
#include <vector>

#include "../casting.hpp"
#include "tokenizer.hpp"

/*
 * Regex ::= AlternateRegex
 * AlternateRegex  ::= ConcatRegex ( \+ ConcatRegex)*
 * ConcatRegex     ::= QuantifiedRegex (QuantifiedRegex)*
 * QuantifiedRegex ::= SimpleRegex [\*, \?]
 * SimpleRegex     ::= [\(Regex\), LetterRegex]
 * LetterRegex     ::= [EmptyLetter, GeneralLetter]
 */

namespace rgx {

template <typename Alphabet>
class Regex;

template <typename Alphabet>
class RegexImpl {
 public:
  using CharT = Alphabet::CharT;
  enum RegexKind {
    RK_Letter = 0,
    RK_Empty,
    RK_Kleene,
    RK_Optional,
    RK_Concatenate,
    RK_Alternate,
  };

 protected:
  explicit RegexImpl(RegexKind kind) : kind_(kind) {}

 public:
  // Non copyable
  RegexImpl(const RegexImpl&) = delete;
  RegexImpl& operator=(const RegexImpl&) = delete;

 private:
  const RegexKind kind_;

 public:
  RegexKind GetKind() const { return kind_; }

  virtual ~RegexImpl() { assert(kind_ <= RK_Alternate); };

  static RegexImpl* FromString(std::basic_string_view<CharT> str) {
    Tokenizer<Alphabet> tokenizer{str};
    TokenIterator<Alphabet> it = tokenizer.begin();
    RegexImpl* rgx = FromString(it);

    if (rgx && it != tokenizer.end()) {
      delete rgx;
      return nullptr;
    }

    return rgx;
  }

  virtual RegexImpl* Copy() const {
    assert(false && "Copying bare regex");
    return nullptr;
  }

 protected:
  static RegexImpl* FromString(TokenIterator<Alphabet>& it);
};

template <typename Alphabet>
class RegexLetter : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  uint64_t m_letter_;
  explicit RegexLetter(uint64_t letter)
      : RegexImpl<Alphabet>(RegexImpl<Alphabet>::RK_Letter),
        m_letter_(letter) {}

 public:
  Alphabet::CharT GetLetterChr() const { return Alphabet::Chr(m_letter_); }

  ~RegexLetter() override = default;

  static bool Classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->GetKind() == RegexImpl<Alphabet>::RK_Letter;
  }

  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::Letter) {
      auto* ret = new RegexLetter(it->chr);
      ++it;
      return ret;
    }
    return nullptr;
  }

  RegexLetter* Copy() const override { return new RegexLetter(m_letter_); }
};

template <typename Alphabet>
class RegexEmpty : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  RegexEmpty() : RegexImpl<Alphabet>(RegexImpl<Alphabet>::RK_Empty) {}

 public:
  ~RegexEmpty() override = default;

  static bool Classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->GetKind() == RegexImpl<Alphabet>::RK_Empty;
  }

  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::Empty) {
      auto* ret = new RegexEmpty();
      ++it;
      return ret;
    }
    return nullptr;
  }

  RegexEmpty* Copy() const override { return new RegexEmpty; }
};

template <typename Alphabet>
class RegexSimple : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;

 public:
  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::LBracket) {
      TokenIterator<Alphabet> backup_it = it;
      ++it;
      RegexImpl<Alphabet>* regex = RegexImpl<Alphabet>::FromString(it);

      if (it->type != RegexToken<Alphabet>::Type::RBracket) {
        delete regex;
        it = backup_it;
        return nullptr;
      }

      ++it;
      return regex;
    }

    RegexImpl<Alphabet>* rgx = RegexLetter<Alphabet>::FromString(it);
    if (!rgx) {
      rgx = RegexEmpty<Alphabet>::FromString(it);
    }
    return rgx;
  }
};

template <typename Alphabet>
class RegexQuantified : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  RegexImpl<Alphabet>* m_regex_;
  RegexQuantified(RegexImpl<Alphabet>* regex,
                  typename RegexImpl<Alphabet>::RegexKind kind)
      : RegexImpl<Alphabet>(kind), m_regex_(regex) {
    assert(Classof(this));
  }

 public:
  const RegexImpl<Alphabet>* GetSubregex() const { return m_regex_; }

  ~RegexQuantified() override { delete m_regex_; }

  static bool Classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->GetKind() == RegexImpl<Alphabet>::RK_Kleene ||
           regex->GetKind() == RegexImpl<Alphabet>::RK_Optional;
  }

  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    TokenIterator<Alphabet> backup = it;
    RegexImpl<Alphabet>* regex = RegexSimple<Alphabet>::FromString(it);
    if (!regex) {
      it = backup;
      return nullptr;
    }

    if (it->type == RegexToken<Alphabet>::Type::QuestionMark) {
      ++it;
      return new RegexQuantified(regex, RegexImpl<Alphabet>::RK_Optional);
    }

    if (it->type == RegexToken<Alphabet>::Type::KleeneStar) {
      ++it;
      return new RegexQuantified(regex, RegexImpl<Alphabet>::RK_Kleene);
    }

    return regex;
  }

  RegexQuantified* Copy() const override {
    return new RegexQuantified(m_regex_->Copy(), this->GetKind());
  }
};

template <typename Alphabet>
class RegexConcatenate : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  std::vector<RegexImpl<Alphabet>*> regex_;
  RegexConcatenate()
      : RegexImpl<Alphabet>(RegexImpl<Alphabet>::RK_Concatenate) {}

 public:
  const std::vector<RegexImpl<Alphabet>*>& GetSubregex() const {
    return regex_;
  }
  std::vector<RegexImpl<Alphabet>*>& GetSubregex() { return regex_; }

  ~RegexConcatenate() override {
    for (auto m_regex : regex_) {
      delete m_regex;
    }
  }

  static bool Classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->GetKind() == RegexImpl<Alphabet>::RK_Concatenate;
  }

  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    TokenIterator<Alphabet> backup = it;
    RegexImpl<Alphabet>* regex1 = RegexQuantified<Alphabet>::FromString(it);
    if (!regex1) {
      it = backup;
      return nullptr;
    }

    backup = it;

    RegexImpl<Alphabet>* regex2 = RegexQuantified<Alphabet>::FromString(it);
    if (!regex2) {
      it = backup;
      return regex1;
    }

    backup = it;
    auto* rgx = new RegexConcatenate<Alphabet>;
    rgx->regex_.push_back(regex1);
    rgx->regex_.push_back(regex2);

    while (RegexImpl<Alphabet>* sub_regex =
               RegexQuantified<Alphabet>::FromString(it)) {
      rgx->regex_.push_back(sub_regex);
      backup = it;
    }

    it = backup;
    return rgx;
  }

  RegexConcatenate* Copy() const override {
    auto* copy = new RegexConcatenate();
    for (auto* sub : regex_) {
      copy->regex_.push_back(sub->Copy());
    }
    return copy;
  }
};

template <typename Alphabet>
class RegexAlternate : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  std::vector<RegexImpl<Alphabet>*> regex_;
  RegexAlternate() : RegexImpl<Alphabet>(RegexImpl<Alphabet>::RK_Alternate) {}

 public:
  const std::vector<RegexImpl<Alphabet>*>& GetSubregex() const {
    return regex_;
  }
  std::vector<RegexImpl<Alphabet>*>& GetSubregex() { return regex_; }

  ~RegexAlternate() override {
    for (auto m_regex : regex_) {
      delete m_regex;
    }
  }

  static bool Classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->GetKind() == RegexImpl<Alphabet>::RK_Alternate;
  }

  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    TokenIterator<Alphabet> backup = it;
    RegexImpl<Alphabet>* regex1 = RegexConcatenate<Alphabet>::FromString(it);
    if (!regex1) {
      it = backup;
      return nullptr;
    }

    backup = it;

    if (it->type != RegexToken<Alphabet>::Type::Alternate) {
      return regex1;
    }

    auto* rgx = new RegexAlternate<Alphabet>;
    rgx->regex_.push_back(regex1);

    while (it->type == RegexToken<Alphabet>::Type::Alternate) {
      ++it;
      RegexImpl<Alphabet>* sub_regex =
          RegexConcatenate<Alphabet>::FromString(it);

      if (!sub_regex) {
        break;
      }

      rgx->regex_.push_back(sub_regex);
      backup = it;
    }

    it = backup;
    return rgx;
  }

  RegexAlternate* Copy() const override {
    auto* copy = new RegexAlternate();
    for (auto* sub : regex_) {
      copy->regex_.push_back(sub->Copy());
    }
    return copy;
  }
};

template <typename Alphabet>
RegexImpl<Alphabet>* RegexImpl<Alphabet>::FromString(
    TokenIterator<Alphabet>& it) {
  return RegexAlternate<Alphabet>::FromString(it);
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexImpl<Alphabet>& rgx);

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexLetter<Alphabet>& rgx) {
  typename Alphabet::CharT letter = rgx.GetLetterChr();
  if (Alphabet::NeedEscape(letter)) {
    out << Alphabet::kEscapeChar;
  }
  out << letter;
  return out;
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexQuantified<Alphabet>& rgx) {
  if (rgx.GetSubregex()->GetKind() >= rgx.GetKind()) {
    out << Alphabet::kLBracket;
  }

  out << *rgx.GetSubregex();

  if (rgx.GetSubregex()->GetKind() >= rgx.GetKind()) {
    out << Alphabet::kRBracket;
  }

  out << (rgx.GetKind() == RegexImpl<Alphabet>::RK_Kleene
              ? Alphabet::kStar
              : Alphabet::kQuestionMark);
  return out;
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexConcatenate<Alphabet>& rgx) {
  for (const auto* sub : rgx.GetSubregex()) {
    if (sub->GetKind() >= rgx.GetKind()) {
      out << Alphabet::kLBracket;
    }
    out << *sub;
    if (sub->GetKind() >= rgx.GetKind()) {
      out << Alphabet::kRBracket;
    }
  }
  return out;
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexAlternate<Alphabet>& rgx) {
  out << *rgx.GetSubregex()[0];
  for (size_t i = 1; i < rgx.GetSubregex().size(); ++i) {
    out << Alphabet::kPlus << *rgx.GetSubregex()[i];
  }
  return out;
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexImpl<Alphabet>& rgx) {
  switch (rgx.GetKind()) {
    case RegexImpl<Alphabet>::RK_Letter:
      out << *mgk::Cast<RegexLetter<Alphabet>>(rgx);
      return out;
    case RegexImpl<Alphabet>::RK_Kleene:
    case RegexImpl<Alphabet>::RK_Optional:
      out << *mgk::Cast<RegexQuantified<Alphabet>>(rgx);
      return out;
    case RegexImpl<Alphabet>::RK_Alternate:
      out << *mgk::Cast<RegexAlternate<Alphabet>>(rgx);
      return out;
    case RegexImpl<Alphabet>::RK_Concatenate:
      out << *mgk::Cast<RegexConcatenate<Alphabet>>(rgx);
      return out;
    case RegexImpl<Alphabet>::RK_Empty:
      assert(mgk::Cast<RegexEmpty<Alphabet>>(rgx));
      out << Alphabet::kEmptyWord;
      return out;
    default:
      assert(false && "Bad regex");
  }
  return out;
}

template <typename Alphabet>
class Regex {
  using CharT = Alphabet::CharT;
  RegexImpl<Alphabet>* impl_ = nullptr;
  size_t* n_owns_ = nullptr;

  explicit Regex(RegexImpl<Alphabet>* rgx)
      : impl_(rgx), n_owns_(new size_t(1)) {
    assert(impl_);
  }

  // MUST be called on every non-const method;
  void Modify() {
    if (*n_owns_ != 1) {
      --*n_owns_;
      n_owns_ = new size_t(1);
      impl_ = impl_->Copy();
    }
  }

  void Forget() {
    assert(*n_owns_ == 1);
    delete n_owns_;
    n_owns_ = nullptr;
    impl_ = nullptr;
  }

 public:
  Regex() : impl_(nullptr) {}

  explicit Regex(std::basic_string_view<typename Alphabet::CharT> str)
      : impl_(RegexImpl<Alphabet>::FromString(str)), n_owns_(new size_t(1)) {
    if (!impl_) {
      throw std::runtime_error("Not a regex");
    }
  }

  Regex(const Regex& oth) : impl_(oth.impl_), n_owns_(oth.n_owns_) {
    (*n_owns_)++;
  }

  Regex& operator=(const Regex& oth) {
    this->~Regex();
    new (this) Regex(oth);
    return *this;
  }

  Regex(Regex&& oth) {
    impl_ = oth.impl_;
    n_owns_ = oth.n_owns_;
    oth.impl_ = nullptr;
    oth.n_owns_ = nullptr;
  }

  Regex& operator=(Regex&& oth) {
    std::swap(impl_, oth.impl_);
    std::swap(n_owns_, oth.n_owns_);
    return *this;
  }

  ~Regex() {
    if (n_owns_ && !--(*n_owns_)) {
      delete n_owns_;
      delete impl_;
    }
  }

  static Regex EmptyString() { return Regex(new RegexEmpty<Alphabet>); }

  static Regex SingeLetter(CharT chr) {
    return Regex(new RegexLetter<Alphabet>(Alphabet::Ord(chr)));
  }

  const RegexImpl<Alphabet>* GetImpl() const { return impl_; }

  Regex& Concat(Regex oth) {
    if (!impl_ || mgk::Isa<RegexEmpty<Alphabet>>(*impl_)) {
      return *this = std::move(oth);
    }

    if (mgk::Isa<RegexEmpty<Alphabet>>(*oth.impl_)) {
      return *this;
    }

    Modify();
    oth.Modify();  // Make oth single-owner;

    if (mgk::Isa<RegexConcatenate<Alphabet>, RegexImpl<Alphabet>>(impl_)) {
      mgk::Cast<RegexConcatenate<Alphabet>>(impl_)->GetSubregex().push_back(
          oth.impl_);
    } else {
      auto* rgx = new RegexConcatenate<Alphabet>;
      rgx->GetSubregex().push_back(impl_);
      rgx->GetSubregex().push_back(oth.impl_);
      impl_ = rgx;
    }

    oth.Forget();
    return *this;
  }

  Regex& Alternate(Regex oth) {
    if (!impl_) {
      return *this = std::move(oth);
    }

    Modify();
    oth.Modify();  // Make oth single-owner;

    if (mgk::Isa<RegexAlternate<Alphabet>, RegexImpl<Alphabet>>(impl_)) {
      mgk::Cast<RegexAlternate<Alphabet>>(impl_)->GetSubregex().push_back(
          oth.impl_);
    } else {
      auto* rgx = new RegexAlternate<Alphabet>;
      rgx->GetSubregex().push_back(impl_);
      rgx->GetSubregex().push_back(oth.impl_);
      impl_ = rgx;
    }

    oth.Forget();
    return *this;
  }

  Regex& Kleene() {
    assert(impl_);
    Modify();
    impl_ =
        new RegexQuantified<Alphabet>(impl_, RegexImpl<Alphabet>::RK_Kleene);
    return *this;
  }

  Regex& Optional() {
    assert(impl_);
    Modify();
    impl_ =
        new RegexQuantified<Alphabet>(impl_, RegexImpl<Alphabet>::RK_Optional);
    return *this;
  }

  // Shallow ==
  bool operator==(const Regex& other) const = default;
  bool operator!=(const Regex& other) const = default;
};
template <typename OStream, typename Alphabet>
OStream& operator<<(OStream& out, const Regex<Alphabet>& rgx) {
  if (!rgx.GetImpl()) {
    out << "\"\"";
    return out;
  }
  out << *rgx.GetImpl();
  return out;
}

}  // namespace rgx

#endif /* REGEX_REGEX_HPP */
