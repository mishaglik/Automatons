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
class Regex {
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
  Regex(RegexKind kind) : Kind(kind) {}

  // Non copyable
  Regex(const Regex&) = delete;
  Regex& operator=(const Regex&) = delete;

 private:
  const RegexKind Kind;

 public:
  RegexKind getKind() const { return Kind; }

  virtual ~Regex() { assert(Kind <= RK_Alternate); };

  static Regex* FromString(std::basic_string_view<CharT> str) {
    Tokenizer<Alphabet> tokenizer{str};
    TokenIterator<Alphabet> it = tokenizer.begin();
    Regex* rgx = FromString(it);

    if (rgx && it != tokenizer.end()) {
      delete rgx;
      return nullptr;
    }

    return rgx;
  }

 protected:
  static Regex* FromString(TokenIterator<Alphabet>& it);
};

template <typename Alphabet>
class RegexLetter : public Regex<Alphabet> {
  uint64_t m_letter;
  RegexLetter(uint64_t letter)
      : Regex<Alphabet>(Regex<Alphabet>::RK_Letter), m_letter(letter) {}

 public:
  Alphabet::CharT getLetterChr() const { return Alphabet::Chr(m_letter); }

  ~RegexLetter() override {}

  static bool classof(const Regex<Alphabet>* regex) {
    return !regex || regex->getKind() == Regex<Alphabet>::RK_Letter;
  }

  static Regex<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::Letter) {
      auto* ret = new RegexLetter(it->chr);
      ++it;
      return ret;
    }
    return nullptr;
  }
};

template <typename Alphabet>
class RegexEmpty : public Regex<Alphabet> {
  RegexEmpty() : Regex<Alphabet>(Regex<Alphabet>::RK_Empty) {}

 public:
  ~RegexEmpty() override {}

  static bool classof(const Regex<Alphabet>* regex) {
    return !regex || regex->getKind() == Regex<Alphabet>::RK_Empty;
  }

  static Regex<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::Empty) {
      auto* ret = new RegexEmpty();
      ++it;
      return ret;
    }
    return nullptr;
  }
};

template <typename Alphabet>
class RegexSimple : public Regex<Alphabet> {
 public:
  static Regex<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    if (it->type == RegexToken<Alphabet>::Type::LBracket) {
      TokenIterator<Alphabet> backupIt = it;
      ++it;
      Regex<Alphabet>* regex = Regex<Alphabet>::FromString(it);

      if (it->type != RegexToken<Alphabet>::Type::RBracket) {
        delete regex;
        it = backupIt;
        return nullptr;
      }

      ++it;
      return regex;
    }

    Regex<Alphabet>* rgx = RegexLetter<Alphabet>::FromString(it);
    if (!rgx) {
      rgx = RegexEmpty<Alphabet>::FromString(it);
    }
    return rgx;
  }
};

template <typename Alphabet>
class RegexQuantified : public Regex<Alphabet> {
  Regex<Alphabet>* m_regex;
  RegexQuantified(Regex<Alphabet>* regex,
                  typename Regex<Alphabet>::RegexKind kind)
      : Regex<Alphabet>(kind), m_regex(regex) {
    assert(classof(this));
  }

 public:
  const Regex<Alphabet>* getSubregex() const { return m_regex; }

  ~RegexQuantified() override { delete m_regex; }

  static bool classof(const Regex<Alphabet>* regex) {
    return !regex || regex->getKind() == Regex<Alphabet>::RK_Kleene ||
           regex->getKind() == Regex<Alphabet>::RK_Optional;
  }

  static Regex<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    TokenIterator<Alphabet> backup = it;
    Regex<Alphabet>* regex = RegexSimple<Alphabet>::FromString(it);
    if (!regex) {
      it = backup;
      return nullptr;
    }

    if (it->type == RegexToken<Alphabet>::Type::QuestionMark) {
      ++it;
      return new RegexQuantified(regex, Regex<Alphabet>::RK_Optional);
    }

    if (it->type == RegexToken<Alphabet>::Type::KleeneStar) {
      ++it;
      return new RegexQuantified(regex, Regex<Alphabet>::RK_Kleene);
    }

    return regex;
  }
};

template <typename Alphabet>
class RegexConcatenate : public Regex<Alphabet> {
  std::vector<Regex<Alphabet>*> m_regexs;
  RegexConcatenate() : Regex<Alphabet>(Regex<Alphabet>::RK_Concatenate) {}

 public:
  const std::vector<Regex<Alphabet>*>& getSubregex() const { return m_regexs; }

  ~RegexConcatenate() override {
    for (auto m_regex : m_regexs) delete m_regex;
  }

  static bool classof(const Regex<Alphabet>* regex) {
    return !regex || regex->getKind() == Regex<Alphabet>::RK_Concatenate;
  }

  static Regex<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    TokenIterator<Alphabet> backup = it;
    Regex<Alphabet>* regex1 = RegexQuantified<Alphabet>::FromString(it);
    if (!regex1) {
      it = backup;
      return nullptr;
    }

    backup = it;

    Regex<Alphabet>* regex2 = RegexQuantified<Alphabet>::FromString(it);
    if (!regex2) {
      it = backup;
      return regex1;
    }

    backup = it;
    auto* rgx = new RegexConcatenate<Alphabet>;
    rgx->m_regexs.push_back(regex1);
    rgx->m_regexs.push_back(regex2);

    while (Regex<Alphabet>* subRegex =
               RegexQuantified<Alphabet>::FromString(it)) {
      rgx->m_regexs.push_back(subRegex);
      backup = it;
    }

    it = backup;
    return rgx;
  }
};

template <typename Alphabet>
class RegexAlternate : public Regex<Alphabet> {
  std::vector<Regex<Alphabet>*> m_regexs;
  RegexAlternate() : Regex<Alphabet>(Regex<Alphabet>::RK_Alternate) {}

 public:
  const std::vector<Regex<Alphabet>*>& getSubregex() const { return m_regexs; }

  ~RegexAlternate() override {
    for (auto m_regex : m_regexs) delete m_regex;
  }

  static bool classof(const Regex<Alphabet>* regex) {
    return !regex || regex->getKind() == Regex<Alphabet>::RK_Alternate;
  }

  static Regex<Alphabet>* FromString(TokenIterator<Alphabet>& it) {
    TokenIterator<Alphabet> backup = it;
    Regex<Alphabet>* regex1 = RegexConcatenate<Alphabet>::FromString(it);
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
      Regex<Alphabet>* subRegex = RegexConcatenate<Alphabet>::FromString(it);

      if (!subRegex) break;

      rgx->m_regexs.push_back(subRegex);
      backup = it;
    }

    it = backup;
    return rgx;
  }
};

template <typename Alphabet>
Regex<Alphabet>* Regex<Alphabet>::FromString(TokenIterator<Alphabet>& it) {
  return RegexAlternate<Alphabet>::FromString(it);
}

template <class OStream, typename Alphabet>
OStream& operator<<(OStream& out, const Regex<Alphabet>& rgx);

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

  out << (rgx.getKind() == Regex<Alphabet>::RK_Kleene ? Alphabet::Star
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
OStream& operator<<(OStream& out, const Regex<Alphabet>& rgx) {
  switch (rgx.getKind()) {
    case Regex<Alphabet>::RK_Letter:
      out << *mgk::cast<RegexLetter<Alphabet>>(rgx);
      return out;
    case Regex<Alphabet>::RK_Kleene:
    case Regex<Alphabet>::RK_Optional:
      out << *mgk::cast<RegexQuantified<Alphabet>>(rgx);
      return out;
    case Regex<Alphabet>::RK_Alternate:
      out << *mgk::cast<RegexAlternate<Alphabet>>(rgx);
      return out;
    case Regex<Alphabet>::RK_Concatenate:
      out << *mgk::cast<RegexConcatenate<Alphabet>>(rgx);
      return out;
    case Regex<Alphabet>::RK_Empty:
      assert(mgk::cast<RegexEmpty<Alphabet>>(rgx));
      out << Alphabet::EmptyWord;
      return out;
    default:
      assert(0 && "Bad regex");
  }
  return out;
}

}  // namespace rgx

#endif /* REGEX_REGEX_HPP */
