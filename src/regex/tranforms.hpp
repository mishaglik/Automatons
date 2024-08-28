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
  if (!regex) {
    return NFSA<Alphabet>{};
  }

  switch (regex->GetKind()) {
    case RegexImpl<Alphabet>::RK_Empty: {
      NFSA<Alphabet> nfsa{};
      auto node = nfsa.CreateNode();
      nfsa.AddTransition(nfsa.Start(), NFSA<Alphabet>::kEpsilon, node);
      nfsa.MakeFinite(node);
      return nfsa;
    }

    case RegexImpl<Alphabet>::RK_Letter: {
      NFSA<Alphabet> nfsa{};
      auto node = nfsa.CreateNode();
      nfsa.AddTransition(
          nfsa.Start(),
          Alphabet::Ord(
              mgk::Cast<RegexLetter<Alphabet>>(regex)->GetLetterChr()),
          node);
      nfsa.MakeFinite(node);
      nfsa.Validate();
      return nfsa;
    }

    case RegexImpl<Alphabet>::RK_Kleene: {
      auto* kleene_regex = mgk::Cast<RegexQuantified<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(kleene_regex->GetSubregex());
      nfsa.Validate();
      nfsa.Kleene();
      return nfsa;
    }
    case RegexImpl<Alphabet>::RK_Optional: {
      auto* opt_regex = mgk::Cast<RegexQuantified<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(opt_regex->GetSubregex());
      nfsa.Optional();
      return nfsa;
    }
    case RegexImpl<Alphabet>::RK_Alternate: {
      auto* alt_regex = mgk::Cast<RegexAlternate<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(alt_regex->GetSubregex()[0]);
      for (size_t i = 1; i < alt_regex->GetSubregex().size(); ++i) {
        nfsa.Alternate(NFAFromRegex(alt_regex->GetSubregex()[i]));
      }
      nfsa.Validate();
      return nfsa;
    }
    case RegexImpl<Alphabet>::RK_Concatenate: {
      auto* cat_regex = mgk::Cast<RegexConcatenate<Alphabet>>(regex);
      NFSA<Alphabet> nfsa = NFAFromRegex(cat_regex->GetSubregex()[0]);
      for (size_t i = 1; i < cat_regex->GetSubregex().size(); ++i) {
        nfsa.Concat(NFAFromRegex(cat_regex->GetSubregex()[i]));
      }
      nfsa.Validate();
      return nfsa;
    }
    default:
      assert(0 && "Bad regex");
      abort();
  }
}
}  // namespace details

template <typename Alphabet>
NFSA<Alphabet> NFAFromRegex(const Regex<Alphabet>& regex) {
  return details::NFAFromRegex(regex.GetImpl());
}

template <typename Alphabet>
FDFA<Alphabet> FDFAFromNFA(const NFSA<Alphabet>& nfa) {
  FDFA<Alphabet> dfa;
  using Vertex = std::vector<typename NFSA<Alphabet>::Node>;
  std::vector<Vertex> vertices = {{nfa.Start()}};

  for (size_t i = 0; i < vertices.size(); ++i) {
    for (uint64_t chr = 1; chr < Alphabet::kSize; ++chr) {
      const Vertex& from = vertices[i];
      Vertex to;
      for (auto node : from) {
        if (nfa.Transitions(node).find(chr) == nfa.Transitions(node).end()) {
          continue;
        }
        to.insert(to.end(), nfa.Transitions(node).at(chr).begin(),
                  nfa.Transitions(node).at(chr).end());
      }

      std::sort(to.begin(), to.end());
      to.resize(std::unique(to.begin(), to.end()) - to.begin());

      size_t pos =
          std::find(vertices.begin(), vertices.end(), to) - vertices.begin();
      if (pos == vertices.size()) {
        vertices.push_back(to);
        dfa.CreateNode();
      }
      dfa.SetTransition(i, chr, pos);
    }

    for (auto node : vertices[i]) {
      if (nfa.IsFinite(node)) {
        dfa.MakeFinite(i);
      }
    }
  }
  return dfa;
}

template <typename Alphabet>
FDFA<Alphabet> Minimize(const FDFA<Alphabet> fdfa) {
  FDFA<Alphabet> mindfa;
  mindfa.CreateNode();
  mindfa.MakeFinite(1);
  std::vector<uint64_t> classes(fdfa.Size());
  for (size_t i = 0; i < classes.size(); ++i) {
    classes[i] = fdfa.IsFinite(i);
  }
  mindfa.SetStart(classes[0]);

  bool added_new_class = true;
  while (added_new_class) {
    added_new_class = false;
    std::vector<uint64_t> new_classes = classes;
    std::vector<bool> inited(mindfa.Size(), 0);
    for (size_t i = 0; i < classes.size(); ++i) {
      if (!inited[classes[i]]) {
        inited[classes[i]] = true;
        for (size_t via = 1; via < fdfa.Transitions(i).size(); ++via) {
          mindfa.SetTransition(classes[i], via,
                               classes[fdfa.Transitions(i)[via]]);
        }
        continue;
      }
      std::array<uint64_t, Alphabet::kSize> trans;
      trans[0] = -1ul;
      for (size_t via = 1; via < fdfa.Transitions(i).size(); ++via) {
        trans[via] = classes[fdfa.Transitions(i)[via]];
      }

      if (trans != mindfa.Transitions(classes[i])) {
        for (size_t j = 0; j < i; ++j) {
          if (classes[j] != classes[i]) {
            continue;
          }
          if (trans == mindfa.Transitions(new_classes[j])) {
            new_classes[i] = new_classes[j];
            assert(new_classes[i] != classes[i]);
            break;
          }
        }

        if (new_classes[i] == classes[i]) {  // Not found
          new_classes[i] = mindfa.Size();
          mindfa.CreateNode();
          if (mindfa.IsFinite(classes[i])) {
            mindfa.MakeFinite(new_classes[i]);
          }
          for (size_t via = 1; via < fdfa.Transitions(i).size(); ++via) {
            mindfa.SetTransition(new_classes[i], via,
                                 classes[fdfa.Transitions(i)[via]]);
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
  std::vector<Regex> rgx_alphabet = {};
  rgx_alphabet.emplace_back(Regex::EmptyString());

  for (size_t i = 1; i < Alphabet::kSize; ++i) {
    rgx_alphabet.emplace_back(Regex::SingeLetter(Alphabet::Chr(i)));
  }

  auto regex_num = [&rgx_alphabet](const Regex& regex) -> uint64_t {
    for (size_t i = 0; i < rgx_alphabet.size(); ++i) {
      if (rgx_alphabet[i] == regex) {
        return i;
      }
    }
    rgx_alphabet.emplace_back(regex);
    return rgx_alphabet.size() - 1;
  };

  NFSA<AnyAlphabet> regex_nfa{};

  for (size_t i = 1; i < dfa.Size(); ++i) {
    regex_nfa.CreateNode();
  }

  for (size_t from = 0; from < dfa.Size(); ++from) {
    for (uint64_t via = 1; via < dfa.Transitions(from).size(); ++via) {
      regex_nfa.AddTransition(from, via, dfa.Transitions(from)[via]);
    }
  }

  regex_nfa.SetStart(dfa.Start());
  auto term = regex_nfa.CreateNode();
  regex_nfa.MakeFinite(term);
  for (size_t x = 0; x < dfa.Size(); ++x) {
    if (dfa.IsFinite(x)) {
      regex_nfa.AddTransition(x, 0, term);
    }
  }

  regex_nfa.OptimizeUnreachableTerm();

  std::vector<std::vector<std::pair<uint64_t, uint64_t>>> reverse_transitions(
      regex_nfa.Size());
  std::vector<std::vector<uint64_t>> loops(regex_nfa.Size());

  for (size_t i = 0; i < regex_nfa.Size(); ++i) {
    for (const auto& [key, trans] : regex_nfa.Transitions(i)) {
      for (uint64_t to : trans) {
        if (to == i) {
          loops[i].push_back(key);
        } else {
          reverse_transitions[to].emplace_back(key, i);
        }
      }
    }
  }

  for (uint64_t via = 0; via < regex_nfa.Size(); ++via) {
    if (via == regex_nfa.Start() || regex_nfa.IsFinite(via)) {
      continue;
    }

    Regex loop_regex;
    bool has_loop = !loops[via].empty();
    if (has_loop) {
      for (uint64_t r : loops[via]) {
        loop_regex.Alternate(rgx_alphabet[r]);
      }
      loop_regex.Kleene();
    }

    for (auto [fromChr, from] : reverse_transitions[via]) {
      assert(from != via);
      if (from < via && from != regex_nfa.Start()) {
        continue;
      }
      for (const auto& [toChr, tos] : regex_nfa.Transitions(via)) {
        for (auto to : tos) {
          if (to == via) {
            continue;
          }
          assert(to > via || to == regex_nfa.Start());
          Regex combo = rgx_alphabet[fromChr];
          if (has_loop) {
            combo.Concat(loop_regex);
          }
          combo.Concat(rgx_alphabet[toChr]);

          if (to != from) {
            bool changed = false;
            do {
              changed = false;
              uint64_t letter = regex_nfa.FindTransition(from, to);
              if (letter != NFSA<AnyAlphabet>::kInvalid) {
                regex_nfa.RemoveTransition(from, letter, to);
                combo.Alternate(rgx_alphabet[letter]);
                changed = true;
              }
            } while (changed);
            std::erase_if(reverse_transitions[to],
                          [=](const std::pair<uint64_t, uint64_t>& x) {
                            return x.second == from;
                          });
          }

          uint64_t regex_idx = regex_num(std::move(combo));

          regex_nfa.AddTransition(from, regex_idx, to);
          if (to == from) {
            loops[to].push_back(regex_idx);
          } else {
            reverse_transitions[to].emplace_back(regex_idx, from);
          }
        }
      }
    }
    regex_nfa.RemoveTransitionsFrom(via);
  }

  Regex final_regex;
  bool has_loop = !loops[regex_nfa.Start()].empty();
  if (has_loop) {
    for (uint64_t r : loops[regex_nfa.Start()]) {
      final_regex.Alternate(rgx_alphabet[r]);
    }
    final_regex.Kleene();
  }
  uint64_t trans = regex_nfa.FindTransition(regex_nfa.Start(), term);
  final_regex.Concat(std::move(rgx_alphabet[trans]));
  return std::move(final_regex);
}

template <typename Alphabet>
FDFA<Alphabet> MDFAFromRegex(Regex<Alphabet> rgx) {
  return Minimize(FDFAFromNFA(NFAFromRegex(rgx).RemoveEpsilonTransitions()));
}

}  // namespace rgx

#endif /* REGEX_TRANFORMS_HPP */
