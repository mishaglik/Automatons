#ifndef REGEX_NFA_HPP
#define REGEX_NFA_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <vector>
namespace rgx {

template <typename Alphabet>
class NFSA {
  using NodeT = std::size_t;
  using CharT = Alphabet::CharT;

 public:
  class Node {
    NodeT m_node;
    explicit Node(NodeT node) : m_node(node) {}
    friend NFSA;

   public:
    Node() = default;
    operator uint64_t() const { return m_node; }
    constexpr auto operator<=>(const Node& oth) const {
      assert(m_node != ~0ul);
      return m_node <=> oth.m_node;
    }

    constexpr bool operator==(const Node& oth) const = default;
    constexpr bool operator!=(const Node& oth) const = default;
    constexpr bool operator<=(const Node& oth) const = default;
    constexpr bool operator>=(const Node& oth) const = default;
    constexpr bool operator<(const Node& oth) const = default;
    constexpr bool operator>(const Node& oth) const = default;
  };

 private:
  std::set<NodeT> m_finite;
  std::vector<std::map<uint64_t, std::vector<Node>>> m_transitions;
  NodeT start_state = 0;
  NodeT m_free_node = 1;

  bool has_transition(NodeT from, uint64_t via, Node to) const {
    if (m_transitions[from].find(via) == m_transitions[from].end()) {
      return false;
    }
    return std::find(m_transitions[from].at(via).begin(),
                     m_transitions[from].at(via).end(),
                     to) != m_transitions[from].at(via).end();
  }

  void add_transition(NodeT from, uint64_t via, Node to) {
    if (!has_transition(from, via, to)) {
      m_transitions[from][via].push_back(to);
    }
  }

  void remove_transition(NodeT from, uint64_t via, Node to) {
    auto it = std::find(m_transitions[from][via].begin(),
                        m_transitions[from][via].end(), to);
    if (it != m_transitions[from][via].end()) {
      std::swap(*m_transitions[from][via].rbegin(), *it);
      m_transitions[from][via].pop_back();
    }
  }

 public:
  static const constexpr NodeT ErrorState = ~0ul;
  static const constexpr uint64_t Epsilon =
      0;  // Zero is terminator for strings so it will be good go NFA
          // transition;

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

  void makeFinite(Node node) { m_finite.insert(node.m_node); }

  bool isFinite(Node node) const {
    return m_finite.find(node.m_node) != m_finite.end();
  }

  void removeFinite(Node node) { m_finite.erase(node.m_node); }

  Node start() const { return Node{start_state}; }

  bool has_transition(Node from, uint64_t via, Node to) const {
    return has_transition(from.m_node, via, to);
  }

  void add_transition(Node from, uint64_t via, Node to) {
    add_transition(from.m_node, via, to);
  }

  void remove_transition(Node from, uint64_t via, Node to) {
    remove_transition(from.m_node, via, to);
  }

  const std::map<uint64_t, std::vector<Node>>& transitions(Node from) const {
    return m_transitions[from.m_node];
  }

  void concat(NFSA other);
  void alternate(NFSA other);
  void kleene();
  void optional();

  Node create_node() {
    m_transitions.emplace_back();
    return Node{m_free_node++};
  }

  void removeEpsilonTransitions();

  void optimizeUnreachable();

  void graphDump(const char* filename) const;

  template <typename OStream>
  OStream& textDump(OStream& out) const {
    out << start_state << '\n' << '\n';

    for (NodeT node : m_finite) {
      out << node << '\n';
    }
    out << "\n";

    for (size_t node = 0; node < size(); ++node) {
      for (auto& [c, trans] : m_transitions[node]) {
        CharT chr = Alphabet::Chr(c);
        for (const Node& to : trans) {
          out << node << ' ' << to.m_node << ' ';
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
  NodeT delta = size();

  for (auto& node : oth.m_transitions) {
    for (auto& [chr, trans] : node) {
      for (auto& to : trans) {
        to.m_node += delta;
      }
    }
  }
  oth.start_state += delta;

  m_transitions.insert(m_transitions.end(),
                       std::make_move_iterator(oth.m_transitions.begin()),
                       std::make_move_iterator(oth.m_transitions.end()));

  m_free_node += oth.size();

  for (NodeT node : m_finite) {
    add_transition(node, Epsilon, Node{oth.start_state});
  }

  m_finite.clear();

  for (NodeT node : oth.m_finite) {
    m_finite.insert(node + delta);
  }
  validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::alternate(NFSA<Alphabet> oth) {
  validate();
  oth.validate();

  NodeT delta = size();
  for (auto& node : oth.m_transitions) {
    for (auto& [chr, trans] : node) {
      for (auto& to : trans) {
        to.m_node += delta;
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
  start_state = newStart.m_node;

  Node newTerm = create_node();

  for (NodeT node : m_finite) {
    add_transition(node, Epsilon, newTerm);
  }

  m_finite.clear();

  for (NodeT node : oth.m_finite) {
    add_transition(node + delta, Epsilon, newTerm);
  }

  makeFinite(newTerm);
  validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::kleene() {
  Node newStart = create_node();

  add_transition(newStart, Epsilon, start());

  for (NodeT node : m_finite) {
    add_transition(node, Epsilon, newStart);
  }

  start_state = newStart.m_node;
  makeFinite(start());
  validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::optional() {
  Node newStart = create_node();

  add_transition(newStart, Epsilon, start());
  start_state = newStart.m_node;

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
    for (NodeT node : m_finite) {
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
        dotout << node << " -> " << to.m_node << "[label=\"";
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
void NFSA<Alphabet>::removeEpsilonTransitions() {
  std::vector<NodeT> worklist;
  std::set<NodeT> reachable;
  for (size_t node = 0; node < size(); ++node) {
    reachable.clear();
    worklist.push_back(node);

    while (!worklist.empty()) {
      NodeT via = worklist.back();
      worklist.pop_back();

      if (reachable.find(via) != reachable.end()) continue;

      reachable.insert(via);

      if (m_transitions[via].find(Epsilon) != m_transitions[via].end()) {
        for (Node to : m_transitions[via].at(Epsilon)) {
          worklist.push_back(to.m_node);
        }
      }
    }

    reachable.erase(node);

    for (NodeT via : reachable) {
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
}

template <typename Alphabet>
void NFSA<Alphabet>::optimizeUnreachable() {
  std::vector<bool> reachable(size(), false);
  std::vector<NodeT> worklist;
  worklist.push_back(start_state);

  while (!worklist.empty()) {
    NodeT node = worklist.back();
    worklist.pop_back();

    if (reachable[node]) continue;

    reachable[node] = true;

    for (auto& [chr, trans] : m_transitions[node]) {
      for (const Node& tp : trans) {
        worklist.push_back(tp.m_node);
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

}  // namespace rgx

#endif /* REGEX_NFA_HPP */
