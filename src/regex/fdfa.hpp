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
  using CharT = typename Alphabet::CharT;
  Node start_state_ = 0;
  std::vector<std::array<Node, Alphabet::kSize>> transitions_;
  std::set<Node> finite_;

 public:
  static const constexpr Node kErrorState = ~0ul;
  static const constexpr uint64_t kEpsilon = 0;

  FDFA() {
    transitions_.emplace_back();
    transitions_.back().fill(kErrorState);
  }

  size_t Size() const { return transitions_.size(); }

  void MakeFinite(Node node) { finite_.insert(node); }

  bool IsFinite(Node node) const { return finite_.find(node) != finite_.end(); }

  void RemoveFinite(Node node) { finite_.erase(node); }

  Node Start() const { return start_state_; }

  void SetStart(Node node) { start_state_ = node; }

  bool HasTransition(Node from, uint64_t via, Node to) const {
    assert(from < Size());
    return transitions_[from][via] == to;
  }

  void RemoveTransition(Node from, uint64_t via) {
    assert(from < Size());
    transitions_[from][via] = kErrorState;
  }

  void SetTransition(Node from, uint64_t via, Node to) {
    assert(from < Size());
    transitions_[from][via] = to;
  }

  const std::array<Node, Alphabet::kSize>& Transitions(Node from) const {
    assert(from < Size());
    return transitions_[from];
  }

  Node CreateNode() {
    transitions_.emplace_back();
    transitions_.back().fill(kErrorState);
    return transitions_.size() - 1;
  }

  void GraphDump(const char* filename) const;

  template <typename OStream>
  OStream& TextDump(OStream& out) const {
    out << start_state_ << '\n' << '\n';

    for (Node node : finite_) {
      out << node << '\n';
    }
    out << "\n";

    for (size_t node = 0; node < Size(); ++node) {
      for (uint64_t c = 1; c < Alphabet::kSize; ++c) {
        Node to = transitions_[node][c];
        CharT chr = Alphabet::Chr(c);
        if (to != kErrorState) {
          out << node << ' ' << to << ' ';
          if (Alphabet::NeedEscape(chr)) {
            out << Alphabet::kEscapeChar;
          }
          out << chr << '\n';
        }
      }
    }

    out << '\n';
    return out;
  }

  void Inverse() {
    std::set<Node> new_finite = {};
    for (size_t i = 0; i < Size(); ++i) {
      if (!IsFinite(i)) {
        new_finite.insert(i);
      }
    }
    finite_.swap(new_finite);
  }
};

template <typename Alphabet>
void FDFA<Alphabet>::GraphDump(const char* filename) const {
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
  if (!finite_.empty()) {
    for (Node node : finite_) {
      dotout << node << " ";
    }

    dotout << ";\n";
  }
  dotout << "node [shape = circle];\n";
  dotout << "S -> " << start_state_ << '\n';

  for (size_t node = 0; node < Size(); ++node) {
    for (uint64_t c = 1; c < Alphabet::kSize; ++c) {
      Node to = transitions_[node][c];
      CharT chr = Alphabet::Chr(c);
      if (to != kErrorState) {
        dotout << node << " -> " << to << "[label=\"";
        if (Alphabet::NeedEscape(chr)) {
          dotout << Alphabet::kEscapeChar;
        }
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
