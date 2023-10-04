#ifndef REGEX_TRANFORMS_HPP
#define REGEX_TRANFORMS_HPP

#include "alphabet.hpp"
#include "fdfa.hpp"
#include "nfa.hpp"
#include "regex.hpp"

namespace rgx {

namespace details {
template <typename Alphabet>
NFSA<Alphabet> NFAFromRegex(const RegexImpl<Alphabet>* regex) {
  if (!regex) return NFSA<Alphabet>{};

  switch (regex->getKind()) {
    case RegexImpl<Alphabet>::RK_Empty: {
      NFSA<Alphabet> nfsa{};
      auto node = nfsa.create_node();
      nfsa.add_transition(nfsa.start(), NFSA<Alphabet>::Epsilon, node);
      nfsa.makeFinite(node);
      return nfsa;
    }

    case RegexImpl<Alphabet>::RK_Letter: {
      NFSA<Alphabet> nfsa{};
      auto node = nfsa.create_node();
      nfsa.add_transition(
          nfsa.start(),
          Alphabet::Ord(
              mgk::cast<RegexLetter<Alphabet>>(regex)->getLetterChr()),
          node);
      nfsa.makeFinite(node);
      nfsa.validate();
      return nfsa;
    }

    case RegexImpl<Alphabet>::RK_Kleene: {
      auto* kleene_regex = mgk::cast<RegexQuantified<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(kleene_regex->getSubregex());
      nfsa.validate();
      nfsa.kleene();
      return nfsa;
    }
    case RegexImpl<Alphabet>::RK_Optional: {
      auto* opt_regex = mgk::cast<RegexQuantified<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(opt_regex->getSubregex());
      nfsa.optional();
      return nfsa;
    }
    case RegexImpl<Alphabet>::RK_Alternate: {
      auto* alt_regex = mgk::cast<RegexAlternate<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(alt_regex->getSubregex()[0]);
      for (size_t i = 1; i < alt_regex->getSubregex().size(); ++i) {
        nfsa.alternate(NFAFromRegex(alt_regex->getSubregex()[i]));
      }
      nfsa.validate();
      return nfsa;
    }
    case RegexImpl<Alphabet>::RK_Concatenate: {
      auto* cat_regex = mgk::cast<RegexConcatenate<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(cat_regex->getSubregex()[0]);
      for (size_t i = 1; i < cat_regex->getSubregex().size(); ++i) {
        nfsa.concat(NFAFromRegex(cat_regex->getSubregex()[i]));
      }
      nfsa.validate();
      return nfsa;
    }
    default:
      assert(0 && "Bad regex");
  }
}
}

template <typename Alphabet>
NFSA<Alphabet> NFAFromRegex(const Regex<Alphabet>& regex) {
  return details::NFAFromRegex(regex.getImpl());
}

template <typename Alphabet>
FDFA<Alphabet> FDFAFromNFA(const NFSA<Alphabet>& nfa) {
  FDFA<Alphabet> dfa;
  using Vertex = std::vector<typename NFSA<Alphabet>::Node>;
  std::vector<Vertex> vertices{{nfa.start()}};

  for (size_t i = 0; i < vertices.size(); ++i) {
    for (uint64_t chr = 1; chr < Alphabet::Size; ++chr) {
      const Vertex& from = vertices[i];
      Vertex to;
      for (auto node : from) {
        if (nfa.transitions(node).find(chr) == nfa.transitions(node).end())
          continue;
        to.insert(to.end(), nfa.transitions(node).at(chr).begin(),
                  nfa.transitions(node).at(chr).end());
      }

      std::sort(to.begin(), to.end());
      to.resize(std::unique(to.begin(), to.end()) - to.begin());

      size_t pos =
          std::find(vertices.begin(), vertices.end(), to) - vertices.begin();
      if (pos == vertices.size()) {
        vertices.push_back(to);
        dfa.create_node();
      }
      dfa.set_transition(i, chr, pos);
    }

    for (auto node : vertices[i]) {
      if (nfa.isFinite(node)) {
        dfa.makeFinite(i);
      }
    }
  }
  return dfa;
}

template <typename Alphabet>
FDFA<Alphabet> Minimize(const FDFA<Alphabet> fdfa) {
  FDFA<Alphabet> mindfa;
  mindfa.create_node();
  mindfa.makeFinite(1);
  std::vector<uint64_t> classes(fdfa.size());
  for(size_t i = 0; i < classes.size(); ++i) {
    classes[i] = fdfa.isFinite(i);
  }
  mindfa.setStart(classes[0]);

  bool added_new_class = true;
  while(added_new_class) {
    added_new_class = false;
    std::vector<uint64_t> new_classes = classes;
    std::vector<bool> inited(mindfa.size(), 0);
    for(size_t i = 0; i < classes.size(); ++i) {
      if(!inited[classes[i]]) {
        inited[classes[i]] = true;
        for(size_t via = 1; via < fdfa.transitions(i).size(); ++via) {
          mindfa.set_transition(classes[i], via, classes[fdfa.transitions(i)[via]]);
        }
        continue;
      }
      std::array<uint64_t, Alphabet::Size> trans;
      trans[0] = -1ul;
      for(size_t via = 1; via < fdfa.transitions(i).size(); ++via) {
        trans[via] = classes[fdfa.transitions(i)[via]];
      }

      if(trans != mindfa.transitions(classes[i])) {
        
        for(size_t j = 0; j < i; ++j) {
          if(classes[j] != classes[i]) continue;
          if(trans == mindfa.transitions(new_classes[j])) {
            new_classes[i] = new_classes[j];
            assert(new_classes[i] != classes[i]);
            break;
          }
        }

        if(new_classes[i] == classes[i]) { // Not found
          new_classes[i] = mindfa.size();
          mindfa.create_node();
          if(mindfa.isFinite(classes[i])) {
            mindfa.makeFinite(new_classes[i]);
          }
          for(size_t via = 1; via < fdfa.transitions(i).size(); ++via) {
            mindfa.set_transition(new_classes[i], via, classes[fdfa.transitions(i)[via]]);
          }
          added_new_class = true;
        }
      }
    }
    classes.swap(new_classes);
  }
  return mindfa;
}

template <typename Alphabet>
Regex<Alphabet> RegexFromFDFA(const FDFA<Alphabet>& dfa) {
  using Regex = Regex<Alphabet>;
  std::vector<Regex> rgxAlphabet{};
  rgxAlphabet.emplace_back(Regex::EmptyString());

  for(size_t i = 1; i < Alphabet::Size; ++i) {
    rgxAlphabet.emplace_back(Regex::SingeLetter(Alphabet::Chr(i)));
  }

  auto RegexNum = [&rgxAlphabet] (const Regex& regex) -> uint64_t  {
    for(size_t i = 0; i < rgxAlphabet.size(); ++i) {
      if(rgxAlphabet[i] == regex) return i;
    }
    rgxAlphabet.emplace_back(regex);
    return rgxAlphabet.size()-1;
  };


  NFSA<AnyAlphabet> regexNFA{};

  for(size_t i = 1; i < dfa.size(); ++i) {
    regexNFA.create_node();
  }

  for(size_t from = 0; from < dfa.size(); ++from) {
    for(uint64_t via = 1; via < dfa.transitions(from).size(); ++via) {
      regexNFA.add_transition(from, via, dfa.transitions(from)[via]);
    }
  }

  regexNFA.setStart(dfa.start());
  auto term = regexNFA.create_node();
  regexNFA.makeFinite(term);
  for(size_t x = 0; x < dfa.size(); ++x) {
    if(dfa.isFinite(x)) {
      regexNFA.add_transition(x, 0, term);
    }
  }

  regexNFA.optimizeUnreachableTerm();

  std::vector<std::vector<std::pair<uint64_t, uint64_t>>> reverse_transitions(regexNFA.size());
  std::vector<std::vector<uint64_t>> loops(regexNFA.size());

  for(size_t i = 0; i < regexNFA.size(); ++i) {
    for(const auto& [key, trans] : regexNFA.transitions(i)) {
      for(uint64_t to : trans) {
        if(to == i) {
          loops[i].push_back(key);
        }
        else {
          reverse_transitions[to].push_back({key, i});
        }
      }
    }
  }

  for(uint64_t via = 0; via < regexNFA.size(); ++via) {
    if(via == regexNFA.start() || regexNFA.isFinite(via))
      continue;

    Regex loopRegex;
    bool hasLoop = !loops[via].empty();
    if(hasLoop) {
      for(uint64_t r : loops[via]) {
        loopRegex.alternate(rgxAlphabet[r]);
      }
      loopRegex.kleene();
    }

    for(auto [fromChr, from] : reverse_transitions[via]) {
      assert(from != via);
      if(from < via && from != regexNFA.start()) continue;
      for(const auto& [toChr, tos] : regexNFA.transitions(via)) {
        for(auto to : tos) {
          if(to == via)
            continue;
          assert(to > via || to == regexNFA.start());
          Regex combo = rgxAlphabet[fromChr];
          if(hasLoop) {
            combo.concat(loopRegex);
          }
          combo.concat(rgxAlphabet[toChr]);

          if(to != from) {
            bool changed = false;
            do {
              changed = false;
              uint64_t letter = regexNFA.find_transition(from, to);
              if(letter != NFSA<AnyAlphabet>::Invalid) {
                regexNFA.remove_transition(from, letter, to);
                combo.alternate(rgxAlphabet[letter]);
                changed = true;
              }
            } while(changed);
            std::erase_if(reverse_transitions[to], [=](const std::pair<uint64_t, uint64_t>& x) {return x.second == from;});
          }

          uint64_t regexNum = RegexNum(std::move(combo));

          regexNFA.add_transition(from, regexNum, to);
          if(to == from) {
            loops[to].push_back(regexNum);
          } else {
            reverse_transitions[to].push_back({regexNum, from});
          }
        }
      }
    }
    regexNFA.remove_transitions_from(via);
  }

  Regex finalRegex;
  bool hasLoop = !loops[regexNFA.start()].empty();
  if(hasLoop) {
    for(uint64_t r : loops[regexNFA.start()]) {
      finalRegex.alternate(rgxAlphabet[r]);
    }
    finalRegex.kleene();
  }
  uint64_t trans = regexNFA.find_transition(regexNFA.start(), term);
  finalRegex.concat(std::move(rgxAlphabet[trans]));
  return std::move(finalRegex);
}

template<typename Alphabet>
FDFA<Alphabet> MDFAFromRegex(Regex<Alphabet> rgx) {
  return Minimize(FDFAFromNFA(NFAFromRegex(rgx).removeEpsilonTransitions()));
} 

}  // namespace rgx

#endif /* REGEX_TRANFORMS_HPP */
