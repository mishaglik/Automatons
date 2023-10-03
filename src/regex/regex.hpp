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

template<typename Alphabet>
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
  RegexImpl(RegexKind kind) : Kind(kind) {}

  // Non copyable
  RegexImpl(const RegexImpl&) = delete;
  RegexImpl& operator=(const RegexImpl&) = delete;

 private:
  const RegexKind Kind;

 public:
  RegexKind getKind() const { return Kind; }

  virtual ~RegexImpl() { assert(Kind <= RK_Alternate); };

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
    assert(0 && "Copying bare regex");
  }

 protected:
  static RegexImpl* FromString(TokenIterator<Alphabet>& it);
};

template <typename Alphabet>
class RegexLetter : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  uint64_t m_letter;
  RegexLetter(uint64_t letter)
      : RegexImpl<Alphabet>(RegexImpl<Alphabet>::RK_Letter), m_letter(letter) {}

 public:
  Alphabet::CharT getLetterChr() const { return Alphabet::Chr(m_letter); }

  ~RegexLetter() override {}

  static bool classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->getKind() == RegexImpl<Alphabet>::RK_Letter;
  }

  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::Letter) {
      auto* ret = new RegexLetter(it->chr);
      ++it;
      return ret;
    }
    return nullptr;
  }

  RegexLetter* Copy() const override{
    return new RegexLetter(m_letter);
  }

};

template <typename Alphabet>
class RegexEmpty : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  RegexEmpty() : RegexImpl<Alphabet>(RegexImpl<Alphabet>::RK_Empty) {}

 public:
  ~RegexEmpty() override {}

  static bool classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->getKind() == RegexImpl<Alphabet>::RK_Empty;
  }

  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::Empty) {
      auto* ret = new RegexEmpty();
      ++it;
      return ret;
    }
    return nullptr;
  }

  RegexEmpty* Copy() const override{
    return new RegexEmpty;
  }
};

template <typename Alphabet>
class RegexSimple : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
 public:
  static RegexImpl<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::LBracket) {
      TokenIterator<Alphabet> backupIt = it;
      ++it;
      RegexImpl<Alphabet>* regex = RegexImpl<Alphabet>::FromString(it);

      if (it->type != RegexToken<Alphabet>::Type::RBracket) {
        delete regex;
        it = backupIt;
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
  RegexImpl<Alphabet>* m_regex;
  RegexQuantified(RegexImpl<Alphabet>* regex,
                  typename RegexImpl<Alphabet>::RegexKind kind)
      : RegexImpl<Alphabet>(kind), m_regex(regex) {
    assert(classof(this));
  }

 public:
  const RegexImpl<Alphabet>* getSubregex() const { return m_regex; }

  ~RegexQuantified() override { delete m_regex; }

  static bool classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->getKind() == RegexImpl<Alphabet>::RK_Kleene ||
           regex->getKind() == RegexImpl<Alphabet>::RK_Optional;
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
    return new RegexQuantified(m_regex->Copy(), this->getKind());
  }
};

template <typename Alphabet>
class RegexConcatenate : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  std::vector<RegexImpl<Alphabet>*> m_regexs;
  RegexConcatenate() : RegexImpl<Alphabet>(RegexImpl<Alphabet>::RK_Concatenate) {}

 public:
  const std::vector<RegexImpl<Alphabet>*>& getSubregex() const { return m_regexs; }
        std::vector<RegexImpl<Alphabet>*>& getSubregex()       { return m_regexs; }

  ~RegexConcatenate() override {
    for (auto m_regex : m_regexs) delete m_regex;
  }

  static bool classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->getKind() == RegexImpl<Alphabet>::RK_Concatenate;
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
    rgx->m_regexs.push_back(regex1);
    rgx->m_regexs.push_back(regex2);

    while (RegexImpl<Alphabet>* subRegex =
               RegexQuantified<Alphabet>::FromString(it)) {
      rgx->m_regexs.push_back(subRegex);
      backup = it;
    }

    it = backup;
    return rgx;
  }

  RegexConcatenate* Copy() const override {
    auto* copy = new RegexConcatenate();
    for(auto* sub : m_regexs) {
      copy->m_regexs.push_back(sub->Copy());
    }
    return copy;
  }
};

template <typename Alphabet>
class RegexAlternate : public RegexImpl<Alphabet> {
  friend class Regex<Alphabet>;
  std::vector<RegexImpl<Alphabet>*> m_regexs;
  RegexAlternate() : RegexImpl<Alphabet>(RegexImpl<Alphabet>::RK_Alternate) {}

 public:
  const std::vector<RegexImpl<Alphabet>*>& getSubregex() const { return m_regexs; }
        std::vector<RegexImpl<Alphabet>*>& getSubregex()       { return m_regexs; }

  ~RegexAlternate() override {
    for (auto m_regex : m_regexs) delete m_regex;
  }

  static bool classof(const RegexImpl<Alphabet>* regex) {
    return !regex || regex->getKind() == RegexImpl<Alphabet>::RK_Alternate;
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
    rgx->m_regexs.push_back(regex1);

    while (it->type == RegexToken<Alphabet>::Type::Alternate) {
      ++it;
      RegexImpl<Alphabet>* subRegex = RegexConcatenate<Alphabet>::FromString(it);

      if (!subRegex) break;

      rgx->m_regexs.push_back(subRegex);
      backup = it;
    }

    it = backup;
    return rgx;
  }


  RegexAlternate* Copy() const override {
    auto* copy = new RegexAlternate();
    for(auto* sub : m_regexs) {
      copy->m_regexs.push_back(sub->Copy());
    }
    return copy;
  }
};

template <typename Alphabet>
RegexImpl<Alphabet>* RegexImpl<Alphabet>::FromString(TokenIterator<Alphabet>& it) {
  return RegexAlternate<Alphabet>::FromString(it);
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexImpl<Alphabet>& rgx);

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexLetter<Alphabet>& rgx) {
  typename Alphabet::CharT letter = rgx.getLetterChr();
  if (Alphabet::NeedEscape(letter)) {
    out << Alphabet::EscapeChar;
  }
  out << letter;
  return out;
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexQuantified<Alphabet>& rgx) {
  if (rgx.getSubregex()->getKind() >= rgx.getKind()) {
    out << Alphabet::LBracket;
  }

  out << *rgx.getSubregex();

  if (rgx.getSubregex()->getKind() >= rgx.getKind()) {
    out << Alphabet::RBracket;
  }

  out << (rgx.getKind() == RegexImpl<Alphabet>::RK_Kleene ? Alphabet::Star
                                                      : Alphabet::QuestionMark);
  return out;
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexConcatenate<Alphabet>& rgx) {
  for (const auto* sub : rgx.getSubregex()) {
    if (sub->getKind() >= rgx.getKind()) {
      out << Alphabet::LBracket;
    }
    out << *sub;
    if (sub->getKind() >= rgx.getKind()) {
      out << Alphabet::RBracket;
    }
  }
  return out;
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexAlternate<Alphabet>& rgx) {
  out << *rgx.getSubregex()[0];
  for (size_t i = 1; i < rgx.getSubregex().size(); ++i) {
    out << Alphabet::Plus << *rgx.getSubregex()[i];
  }
  return out;
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const RegexImpl<Alphabet>& rgx) {
  switch (rgx.getKind()) {
    case RegexImpl<Alphabet>::RK_Letter:
      out << *mgk::cast<RegexLetter<Alphabet>>(rgx);
      return out;
    case RegexImpl<Alphabet>::RK_Kleene:
    case RegexImpl<Alphabet>::RK_Optional:
      out << *mgk::cast<RegexQuantified<Alphabet>>(rgx);
      return out;
    case RegexImpl<Alphabet>::RK_Alternate:
      out << *mgk::cast<RegexAlternate<Alphabet>>(rgx);
      return out;
    case RegexImpl<Alphabet>::RK_Concatenate:
      out << *mgk::cast<RegexConcatenate<Alphabet>>(rgx);
      return out;
    case RegexImpl<Alphabet>::RK_Empty:
      assert(mgk::cast<RegexEmpty<Alphabet>>(rgx));
      out << Alphabet::EmptyWord;
      return out;
    default:
      assert(0 && "Bad regex");
  }
  return out;
}

template<typename Alphabet>
class Regex {
  using CharT = Alphabet::CharT;
  RegexImpl<Alphabet>* impl = nullptr;
  size_t* n_owns            = nullptr;

  Regex(RegexImpl<Alphabet>* rgx) : impl(rgx), n_owns(new size_t(1)) {
    assert(impl);
  }

  // MUST be called on every non-const method;
  void modify() {
    if(*n_owns != 1) {
      --*n_owns;
      n_owns = new size_t(1);
      impl = impl->Copy();
    }
  }

  void forget() {
    assert(*n_owns == 1);
    delete n_owns;
    n_owns = nullptr;
    impl   = nullptr;
  }

public:
  explicit Regex(std::basic_string_view<typename Alphabet::CharT> str) : impl(RegexImpl<Alphabet>::FromString(str)), n_owns(new size_t(1)) {
    if(!impl) {
      throw std::runtime_error("Not a regex");
    }
  }

  Regex(const Regex& oth) : impl(oth.impl), n_owns(oth.n_owns) {
    (*n_owns)++;
  }

  Regex& operator=(const Regex& oth) {
    this->~Regex();
    new(this) Regex(oth);
    return *this;
  }

  Regex(Regex&& oth) {
    impl   = oth.impl;
    n_owns = oth.n_owns;
    oth.impl   = nullptr;
    oth.n_owns = nullptr;
  }

  Regex& operator=(Regex&& oth) {
    impl   = oth.impl;
    n_owns = oth.n_owns;
    oth.impl   = nullptr;
    oth.n_owns = nullptr;
    return *this;
  }

  ~Regex() {
    if(n_owns && !--(*n_owns)) {
      delete n_owns;
      delete impl;
    }
  }

  static Regex EmptyString() {
    return Regex(new RegexEmpty<Alphabet>);
  }

  static Regex SingeLetter(CharT chr) {
    return Regex(new RegexLetter<Alphabet>(chr));
  }

  const RegexImpl<Alphabet>* getImpl() const {return impl;}

  Regex& concat(Regex oth) {
    modify();
    oth.modify(); // Make oth single-owner;

    if(mgk::isa<RegexConcatenate<Alphabet>, RegexImpl<Alphabet>>(impl)) {
      mgk::cast<RegexConcatenate<Alphabet>>(impl)->getSubregex().push_back(oth.impl);
    } else {
      auto* rgx = new RegexConcatenate<Alphabet>;
      rgx->getSubregex().push_back(impl);
      rgx->getSubregex().push_back(oth.impl);
      impl = rgx;
    }

    oth.forget();
    return *this;
  }

  Regex& alternate(Regex oth) {
    modify();
    oth.modify(); // Make oth single-owner;

    if(mgk::isa<RegexAlternate<Alphabet>, RegexImpl<Alphabet>>(impl)) {
      mgk::cast<RegexAlternate<Alphabet>>(impl)->getSubregex().push_back(oth.impl);
    } else {
      auto* rgx = new RegexAlternate<Alphabet>;
      rgx->getSubregex().push_back(impl);
      rgx->getSubregex().push_back(oth.impl);
      impl = rgx;
    }

    oth.forget();
    return *this;
  }
  
  Regex& kleene() {
    modify();
    impl = new RegexQuantified<Alphabet>(impl, RegexImpl<Alphabet>::RK_Kleene);
    return *this;
  }

  Regex& optional() {
    modify();
    impl = new RegexQuantified<Alphabet>(impl, RegexImpl<Alphabet>::RK_Optional);
    return *this;
  }

  // Shallow ==
  bool operator==(const Regex& other) const = default;
  bool operator!=(const Regex& other) const = default;
};


}  // namespace rgx

#endif /* REGEX_REGEX_HPP */
