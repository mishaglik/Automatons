#ifndef REGEX_TRANFORMS_HPP
#define REGEX_TRANFORMS_HPP

#include "fdfa.hpp"
#include "nfa.hpp"
#include "regex.hpp"

namespace rgx {

template <typename Alphabet>
NFSA<Alphabet> NFAFromRegex(const Regex<Alphabet>* regex) {
  if (!regex) return NFSA<Alphabet>{};

  switch (regex->getKind()) {
    case Regex<Alphabet>::RK_Empty: {
      NFSA<Alphabet> nfsa{};
      auto node = nfsa.create_node();
      nfsa.add_transition(nfsa.start(), NFSA<Alphabet>::Epsilon, node);
      nfsa.makeFinite(node);
      return nfsa;
    }

    case Regex<Alphabet>::RK_Letter: {
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

    case Regex<Alphabet>::RK_Kleene: {
      auto* kleene_regex = mgk::cast<RegexQuantified<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(kleene_regex->getSubregex());
      nfsa.validate();
      nfsa.kleene();
      return nfsa;
    }
    case Regex<Alphabet>::RK_Optional: {
      auto* opt_regex = mgk::cast<RegexQuantified<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(opt_regex->getSubregex());
      nfsa.optional();
      return nfsa;
    }
    case Regex<Alphabet>::RK_Alternate: {
      auto* alt_regex = mgk::cast<RegexAlternate<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(alt_regex->getSubregex()[0]);
      for (size_t i = 1; i < alt_regex->getSubregex().size(); ++i) {
        nfsa.alternate(NFAFromRegex(alt_regex->getSubregex()[i]));
      }
      nfsa.validate();
      return nfsa;
    }
    case Regex<Alphabet>::RK_Concatenate: {
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
        for(size_t via = 0; via < fdfa.transitions(i).size(); ++via) {
          mindfa.set_transition(classes[i], via, classes[fdfa.transitions(i)[via]]);
        }
        continue;
      }
      std::array<uint64_t, Alphabet::Size> trans;
      for(size_t via = 0; via < fdfa.transitions(i).size(); ++via) {
        trans[via] = classes[fdfa.transitions(i)[via]];
      }

      if(trans != mindfa.transitions(classes[i])) {
        
        for(size_t j = 0; j < classes.size(); ++j) {
          if(classes[j] != classes[i]) continue;
          if(trans == mindfa.transitions(classes[j])) {
            new_classes[i] = new_classes[j];
            break;
          }
        }

        if(new_classes[i] == classes[i]) { // Not found
          new_classes[i] = mindfa.size();
          mindfa.create_node();
          if(mindfa.isFinite(classes[i])) {
            mindfa.makeFinite(new_classes[i]);
          }
          added_new_class = true;
        }
      }
    }
    classes.swap(new_classes);
  }
  return mindfa;
}

}  // namespace rgx

#endif /* REGEX_TRANFORMS_HPP */
