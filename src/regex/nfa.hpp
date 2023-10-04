#ifndef REGEX_NFA_HPP
#define REGEX_NFA_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <vector>
namespace rgx {

template <typename Alphabet>
class NFSA {
  using CharT = Alphabet::CharT;
 public:
  using Node = std::size_t;
 private:
  std::set<Node> m_finite;
  std::vector<std::map<uint64_t, std::vector<Node>>> m_transitions;
  Node start_state = 0;
  Node m_free_node = 1;

  
 public:
  static const constexpr Node ErrorState = ~0ul;
  static const constexpr uint64_t Epsilon = 0;  // Zero is terminator for strings so it will be good go NFA
  static const constexpr uint64_t Invalid = ~0ul;

  NFSA() { m_transitions.emplace_back(); }

  void validate() const {
    for (size_t node = 0; node < size(); ++node) {
      for (auto& [c, trans] : m_transitions[node]) {
        for (const Node& to : trans) {
          assert(to < size());
        }
      }
    }
  }

  size_t size() const { return m_free_node; }

  void makeFinite(Node node) { m_finite.insert(node); }

  bool isFinite(Node node) const {
    return m_finite.find(node) != m_finite.end();
  }

  void removeFinite(Node node) { m_finite.erase(node); }

  Node start() const { return Node{start_state}; }

  void setStart(Node st) { start_state = st; }

  const std::map<uint64_t, std::vector<Node>>& transitions(Node from) const {
    return m_transitions[from];
  }

  bool has_transition(Node from, uint64_t via, Node to) const {
    assert(from < m_transitions.size());
    if (m_transitions[from].find(via) == m_transitions[from].end()) {
      return false;
    }
    return std::find(m_transitions[from].at(via).begin(),
                     m_transitions[from].at(via).end(),
                     to) != m_transitions[from].at(via).end();
  }

  void add_transition(Node from, uint64_t via, Node to) {
    if (!has_transition(from, via, to)) {
      m_transitions[from][via].push_back(to);
    }
  }

  void remove_transition(Node from, uint64_t via, Node to) {
    auto it = std::find(m_transitions[from][via].begin(),
                        m_transitions[from][via].end(), to);
    if (it != m_transitions[from][via].end()) {
      std::swap(*m_transitions[from][via].rbegin(), *it);
      m_transitions[from][via].pop_back();
    }
  }

  uint64_t find_transition(Node from, Node to) {
    for(const auto& [key, trans] : m_transitions[from]) {
      for(uint64_t dest : trans) {
        if(dest == to) {
          return key;
        }
      }
    }
    return Invalid;
  }

  void remove_transitions_from(Node from) {
    m_transitions[from].clear();
  }


  void concat(NFSA other);
  void alternate(NFSA other);
  void kleene();
  void optional();

  Node create_node() {
    m_transitions.emplace_back();
    return Node{m_free_node++};
  }

  NFSA& removeEpsilonTransitions();

  void optimizeUnreachable();
  void optimizeUnreachableTerm();

  void graphDump(const char* filename) const;

  template <typename OStream>
  OStream& textDump(OStream& out) const {
    out << start_state << '\n' << '\n';

    for (Node node : m_finite) {
      out << node << '\n';
    }
    out << "\n";

    for (size_t node = 0; node < size(); ++node) {
      for (auto& [c, trans] : m_transitions[node]) {
        CharT chr = Alphabet::Chr(c);
        for (const Node& to : trans) {
          out << node << ' ' << to << ' ';
          if (c != Epsilon) {
            if (Alphabet::NeedEscape(chr)) out << Alphabet::EscapeChar;
            out << chr << '\n';
          }
        }
      }
    }
    out << '\n';
    return out;
  }
};

template <typename Alphabet>
void NFSA<Alphabet>::concat(NFSA<Alphabet> oth) {
  validate();
  oth.validate();
  Node delta = size();

  for (auto& node : oth.m_transitions) {
    for (auto& [chr, trans] : node) {
      for (auto& to : trans) {
        to += delta;
      }
    }
  }
  oth.start_state += delta;

  m_transitions.insert(m_transitions.end(),
                       std::make_move_iterator(oth.m_transitions.begin()),
                       std::make_move_iterator(oth.m_transitions.end()));

  m_free_node += oth.size();

  for (Node node : m_finite) {
    add_transition(node, Epsilon, Node{oth.start_state});
  }

  m_finite.clear();

  for (Node node : oth.m_finite) {
    m_finite.insert(node + delta);
  }
  validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::alternate(NFSA<Alphabet> oth) {
  validate();
  oth.validate();

  Node delta = size();
  for (auto& node : oth.m_transitions) {
    for (auto& [chr, trans] : node) {
      for (auto& to : trans) {
        to += delta;
      }
    }
  }
  oth.start_state += delta;

  m_transitions.insert(m_transitions.end(),
                       std::make_move_iterator(oth.m_transitions.begin()),
                       std::make_move_iterator(oth.m_transitions.end()));

  m_free_node += oth.size();

  Node newStart = create_node();
  add_transition(newStart, Epsilon, start());
  add_transition(newStart, Epsilon, Node{oth.start_state});
  start_state = newStart;

  Node newTerm = create_node();

  for (Node node : m_finite) {
    add_transition(node, Epsilon, newTerm);
  }

  m_finite.clear();

  for (Node node : oth.m_finite) {
    add_transition(node + delta, Epsilon, newTerm);
  }

  makeFinite(newTerm);
  validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::kleene() {
  Node newStart = create_node();

  add_transition(newStart, Epsilon, start());

  for (Node node : m_finite) {
    add_transition(node, Epsilon, newStart);
  }

  start_state = newStart;
  makeFinite(start());
  validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::optional() {
  Node newStart = create_node();

  add_transition(newStart, Epsilon, start());
  start_state = newStart;

  makeFinite(start());
  validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::graphDump(const char* filename) const {
  std::string tmp_name = "/tmp/";
  tmp_name += filename;
  tmp_name += ".dot";
  std::ofstream dotout(tmp_name.c_str());

  dotout << "digraph NFSA_" << this
         << "{\n"
            "fontname=\"Helvetica,Arial,sans-serif\"\n"
            "node [fontname=\"Helvetica,Arial,sans-serif\"]\n"
            "edge [fontname=\"Helvetica,Arial,sans-serif\"]\n"
            "rankdir=LR;\n"
            "S [style = invis];"
            "node [shape = doublecircle];\n";
  if (!m_finite.empty()) {
    for (Node node : m_finite) {
      dotout << node << " ";
    }

    dotout << ";\n";
  }
  dotout << "node [shape = circle];\n";
  dotout << "S -> " << start_state << '\n';

  for (size_t node = 0; node < size(); ++node) {
    for (auto& [c, trans] : m_transitions[node]) {
      CharT chr = Alphabet::Chr(c);
      for (const Node& to : trans) {
        dotout << node << " -> " << to << "[label=\"";
        if (c != Epsilon) {
          if (Alphabet::NeedEscape(chr)) dotout << Alphabet::EscapeChar;
          dotout << chr;
        } else
          dotout << "\\\"\\\"";
        dotout << "\"];\n";
      }
    }
  }

  dotout << "}\n";
  dotout.close();

  std::string command = "dot " + tmp_name + " -T png -o " + filename;
  std::system(command.c_str());
}

template <typename Alphabet>
NFSA<Alphabet>& NFSA<Alphabet>::removeEpsilonTransitions() {
  std::vector<Node> worklist;
  std::set<Node> reachable;
  for (size_t node = 0; node < size(); ++node) {
    reachable.clear();
    worklist.push_back(node);

    while (!worklist.empty()) {
      Node via = worklist.back();
      worklist.pop_back();

      if (reachable.find(via) != reachable.end()) continue;

      reachable.insert(via);

      if (m_transitions[via].find(Epsilon) != m_transitions[via].end()) {
        for (Node to : m_transitions[via].at(Epsilon)) {
          worklist.push_back(to);
        }
      }
    }

    reachable.erase(node);

    for (Node via : reachable) {
      if (m_finite.find(via) != m_finite.end()) {
        m_finite.insert(node);
      }

      for (auto& [chr, trans] : m_transitions[via]) {
        if (chr == Epsilon) continue;
        for (const Node& tp : trans) {
          add_transition(node, chr, tp);
        }
      }
    }

    m_transitions[node].erase(Epsilon);
  }
  optimizeUnreachable();
  validate();
  return *this;
}

template <typename Alphabet>
void NFSA<Alphabet>::optimizeUnreachable() {
  std::vector<bool> reachable(size(), false);
  std::vector<Node> worklist;
  worklist.push_back(start_state);

  while (!worklist.empty()) {
    Node node = worklist.back();
    worklist.pop_back();

    if (reachable[node]) continue;

    reachable[node] = true;

    for (auto& [chr, trans] : m_transitions[node]) {
      for (const Node& tp : trans) {
        worklist.push_back(tp);
      }
    }
  }

  for (size_t i = 0; i < size(); ++i) {
    if (!reachable[i]) {
      m_finite.erase(i);
      m_transitions[i].clear();
    }
  }
}


template <typename Alphabet>
void NFSA<Alphabet>::optimizeUnreachableTerm() {
  std::vector<bool> reachableTerm(size(), false);
  std::vector<bool> visited(size(), false);

  std::function<void(Node)> checkReachableTerm;
  checkReachableTerm = [&, this] (Node node) {
    if(reachableTerm[node]) return;
    visited[node] = true;
    if(isFinite(node)) {
      reachableTerm[node] = true;
      return;
    }

    for(auto& [key, tos] : m_transitions[node]) {
      for(auto& to : tos) {
        if(!visited[to]) checkReachableTerm(to);
        if(reachableTerm[to]) {
          reachableTerm[node] = true;
          return;
        }
      }
    }
  };

  for(size_t i = 0; i < size(); ++i) {
    visited.assign(size(), false);
    checkReachableTerm(i);
  }
  for(size_t i = 0; i < size(); ++i) {
    for(auto& [key, tos] : m_transitions[i]) {
        std::erase_if(tos, [&](Node node) {return !reachableTerm[node];});
    }
  }
}

}  // namespace rgx

#endif /* REGEX_NFA_HPP */
