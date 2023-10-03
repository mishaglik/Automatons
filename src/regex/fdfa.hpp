#ifndef REGEX_FDFA_HPP
#define REGEX_FDFA_HPP

#include <algorithm>
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
class FDFA {
  using Node = std::size_t;
  using CharT = Alphabet::CharT;
  Node start_state = 0;
  std::vector<std::array<Node, Alphabet::Size>> m_transitions;
  std::set<Node> m_finite;

 public:
  static const constexpr Node ErrorState = ~0ul;
  static const constexpr uint64_t Epsilon = 0;

  FDFA() {
    m_transitions.emplace_back();
    m_transitions.back().fill(ErrorState);
  }

  size_t size() const { return m_transitions.size(); }

  void makeFinite(Node node) { m_finite.insert(node); }

  bool isFinite(Node node) const {
    return m_finite.find(node) != m_finite.end();
  }

  void removeFinite(Node node) { m_finite.erase(node); }

  Node start() const { return start_state; }

  void setStart(Node node) { start_state = node; }

  bool has_transition(Node from, uint64_t via, Node to) const {
    assert(from < size());
    return m_transitions[from][via] == to;
  }

  void remove_transition(Node from, uint64_t via) {
    assert(from < size());
    m_transitions[from][via] = ErrorState;
  }

  void set_transition(Node from, uint64_t via, Node to) {
    assert(from < size());
    m_transitions[from][via] = to;
  }

  const std::array<Node, Alphabet::Size>& transitions(Node from) const {
    assert(from < size());
    return m_transitions[from];
  }

  Node create_node() {
    m_transitions.emplace_back();
    m_transitions.back().fill(ErrorState);
    return m_transitions.size() - 1;
  }

  void graphDump(const char* filename) const;

  template <typename OStream>
  OStream& textDump(OStream& out) const {
    out << start_state << '\n' << '\n';

    for (Node node : m_finite) {
      out << node << '\n';
    }
    out << "\n";

    for (size_t node = 0; node < size(); ++node) {
      for (uint64_t c = 1; c < Alphabet::Size; ++c) {
        Node to = m_transitions[node][c];
        CharT chr = Alphabet::Chr(c);
        if (to != ErrorState) {
          out << node << ' ' << to << ' ';
          if (Alphabet::NeedEscape(chr)) out << Alphabet::EscapeChar;
          out << chr << '\n';
        }
      }
    }

    out << '\n';
    return out;
  }

  void inverse() {
    std::set<Node> new_finite = {};
    for (size_t i = 0; i < size(); ++i) {
      if (!isFinite(i)) new_finite.insert(i);
    }
    m_finite.swap(new_finite);
  }
};

template <typename Alphabet>
void FDFA<Alphabet>::graphDump(const char* filename) const {
  std::string tmp_name = "/tmp/";
  tmp_name += filename;
  tmp_name += ".dot";
  std::ofstream dotout(tmp_name.c_str());

  dotout << "digraph FDFA_" << this
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
    for (uint64_t c = 1; c < Alphabet::Size; ++c) {
      Node to = m_transitions[node][c];
      CharT chr = Alphabet::Chr(c);
      if (to != ErrorState) {
        dotout << node << " -> " << to << "[label=\"";
        if (Alphabet::NeedEscape(chr)) dotout << Alphabet::EscapeChar;
        dotout << chr;
        dotout << "\"];\n";
      }
    }
  }

  dotout << "}\n";
  dotout.close();

  std::string command = "dot " + tmp_name + " -T png -o " + filename;
  std::system(command.c_str());
}

}  // namespace rgx

#endif /* REGEX_FDFA_HPP */
    