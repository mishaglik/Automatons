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
  using CharT = typename Alphabet::CharT;

 public:
  using Node = std::size_t;

 private:
  std::set<Node> m_finite_;
  std::vector<std::map<uint64_t, std::vector<Node>>> m_transitions_;
  Node start_state_ = 0;
  Node m_free_node_ = 1;

 public:
  static const constexpr Node kErrorState = ~0ul;
  static const constexpr uint64_t kEpsilon =
      0;  // Zero is terminator for strings so it will be good go NFA
  static const constexpr uint64_t kInvalid = ~0ul;

  NFSA() { m_transitions_.emplace_back(); }

  void Validate() const {
#ifndef NDEBUG
    for (size_t node = 0; node < Size(); ++node) {
      for (auto& [c, trans] : m_transitions_[node]) {
        for (const Node& to : trans) {
          assert(to < Size());
        }
      }
    }
#endif
  }

  size_t Size() const { return m_free_node_; }

  void MakeFinite(Node node) { m_finite_.insert(node); }

  bool IsFinite(Node node) const {
    return m_finite_.find(node) != m_finite_.end();
  }

  void RemoveFinite(Node node) { m_finite_.erase(node); }

  Node Start() const { return Node{start_state_}; }

  void SetStart(Node st) { start_state_ = st; }

  const std::map<uint64_t, std::vector<Node>>& Transitions(Node from) const {
    return m_transitions_[from];
  }

  bool HasTransition(Node from, uint64_t via, Node to) const {
    assert(from < m_transitions_.size());
    if (m_transitions_[from].find(via) == m_transitions_[from].end()) {
      return false;
    }
    return std::find(m_transitions_[from].at(via).begin(),
                     m_transitions_[from].at(via).end(),
                     to) != m_transitions_[from].at(via).end();
  }

  void AddTransition(Node from, uint64_t via, Node to) {
    if (!HasTransition(from, via, to)) {
      m_transitions_[from][via].push_back(to);
    }
  }

  void RemoveTransition(Node from, uint64_t via, Node to) {
    auto it = std::find(m_transitions_[from][via].begin(),
                        m_transitions_[from][via].end(), to);
    if (it != m_transitions_[from][via].end()) {
      std::swap(*m_transitions_[from][via].rbegin(), *it);
      m_transitions_[from][via].pop_back();
    }
  }

  uint64_t FindTransition(Node from, Node to) {
    for (const auto& [key, trans] : m_transitions_[from]) {
      for (uint64_t dest : trans) {
        if (dest == to) {
          return key;
        }
      }
    }
    return kInvalid;
  }

  void RemoveTransitionsFrom(Node from) { m_transitions_[from].clear(); }

  void Concat(NFSA other);
  void Alternate(NFSA other);
  void Kleene();
  void Optional();

  Node CreateNode() {
    m_transitions_.emplace_back();
    return Node{m_free_node_++};
  }

  NFSA& RemoveEpsilonTransitions();

  void OptimizeUnreachable();
  void OptimizeUnreachableTerm();

  void GraphDump(const char* filename) const;

  template <typename OStream>
  OStream& TextDump(OStream& out) const {
    out << start_state_ << '\n' << '\n';

    for (Node node : m_finite_) {
      out << node << '\n';
    }
    out << "\n";

    for (size_t node = 0; node < Size(); ++node) {
      for (auto& [c, trans] : m_transitions_[node]) {
        CharT chr = Alphabet::Chr(c);
        for (const Node& to : trans) {
          out << node << ' ' << to << ' ';
          if (c != kEpsilon) {
            if (Alphabet::NeedEscape(chr)) {
              out << Alphabet::kEscapeChar;
            }
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
void NFSA<Alphabet>::Concat(NFSA<Alphabet> oth) {
  Validate();
  oth.Validate();
  Node delta = Size();

  for (auto& node : oth.m_transitions_) {
    for (auto& [chr, trans] : node) {
      for (auto& to : trans) {
        to += delta;
      }
    }
  }
  oth.start_state_ += delta;

  m_transitions_.insert(m_transitions_.end(),
                        std::make_move_iterator(oth.m_transitions_.begin()),
                        std::make_move_iterator(oth.m_transitions_.end()));

  m_free_node_ += oth.Size();

  for (Node node : m_finite_) {
    AddTransition(node, kEpsilon, Node{oth.start_state_});
  }

  m_finite_.clear();

  for (Node node : oth.m_finite_) {
    m_finite_.insert(node + delta);
  }
  Validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::Alternate(NFSA<Alphabet> oth) {
  Validate();
  oth.Validate();

  Node delta = Size();
  for (auto& node : oth.m_transitions_) {
    for (auto& [chr, trans] : node) {
      for (auto& to : trans) {
        to += delta;
      }
    }
  }
  oth.start_state_ += delta;

  m_transitions_.insert(m_transitions_.end(),
                        std::make_move_iterator(oth.m_transitions_.begin()),
                        std::make_move_iterator(oth.m_transitions_.end()));

  m_free_node_ += oth.Size();

  Node new_start = CreateNode();
  AddTransition(new_start, kEpsilon, Start());
  AddTransition(new_start, kEpsilon, Node{oth.start_state_});
  start_state_ = new_start;

  Node new_term = CreateNode();

  for (Node node : m_finite_) {
    AddTransition(node, kEpsilon, new_term);
  }

  m_finite_.clear();

  for (Node node : oth.m_finite_) {
    AddTransition(node + delta, kEpsilon, new_term);
  }

  MakeFinite(new_term);
  Validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::Kleene() {
  Node new_start = CreateNode();

  AddTransition(new_start, kEpsilon, Start());

  for (Node node : m_finite_) {
    AddTransition(node, kEpsilon, new_start);
  }

  start_state_ = new_start;
  MakeFinite(Start());
  Validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::Optional() {
  Node new_start = CreateNode();

  AddTransition(new_start, kEpsilon, Start());
  start_state_ = new_start;

  MakeFinite(Start());
  Validate();
}

template <typename Alphabet>
void NFSA<Alphabet>::GraphDump(const char* filename) const {
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
  if (!m_finite_.empty()) {
    for (Node node : m_finite_) {
      dotout << node << " ";
    }

    dotout << ";\n";
  }
  dotout << "node [shape = circle];\n";
  dotout << "S -> " << start_state_ << '\n';

  for (size_t node = 0; node < Size(); ++node) {
    for (auto& [c, trans] : m_transitions_[node]) {
      CharT chr = Alphabet::Chr(c);
      for (const Node& to : trans) {
        dotout << node << " -> " << to << "[label=\"";
        if (c != kEpsilon) {
          if (Alphabet::NeedEscape(chr)) {
            dotout << Alphabet::kEscapeChar;
          }
          dotout << chr;
        } else {
          dotout << "\\\"\\\"";
        }
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
NFSA<Alphabet>& NFSA<Alphabet>::RemoveEpsilonTransitions() {
  std::vector<Node> worklist;
  std::set<Node> reachable;
  for (size_t node = 0; node < Size(); ++node) {
    reachable.clear();
    worklist.push_back(node);

    while (!worklist.empty()) {
      Node via = worklist.back();
      worklist.pop_back();

      if (reachable.find(via) != reachable.end()) {
        continue;
      }

      reachable.insert(via);

      if (m_transitions_[via].find(kEpsilon) != m_transitions_[via].end()) {
        for (Node to : m_transitions_[via].at(kEpsilon)) {
          worklist.push_back(to);
        }
      }
    }

    reachable.erase(node);

    for (Node via : reachable) {
      if (m_finite_.find(via) != m_finite_.end()) {
        m_finite_.insert(node);
      }

      for (auto& [chr, trans] : m_transitions_[via]) {
        if (chr == kEpsilon) {
          continue;
        }
        for (const Node& tp : trans) {
          AddTransition(node, chr, tp);
        }
      }
    }

    m_transitions_[node].erase(kEpsilon);
  }
  OptimizeUnreachable();
  Validate();
  return *this;
}

template <typename Alphabet>
void NFSA<Alphabet>::OptimizeUnreachable() {
  std::vector<bool> reachable(Size(), false);
  std::vector<Node> worklist;
  worklist.push_back(start_state_);

  while (!worklist.empty()) {
    Node node = worklist.back();
    worklist.pop_back();

    if (reachable[node]) {
      continue;
    }

    reachable[node] = true;

    for (auto& [chr, trans] : m_transitions_[node]) {
      for (const Node& tp : trans) {
        worklist.push_back(tp);
      }
    }
  }

  for (size_t i = 0; i < Size(); ++i) {
    if (!reachable[i]) {
      m_finite_.erase(i);
      m_transitions_[i].clear();
    }
  }
}

template <typename Alphabet>
void NFSA<Alphabet>::OptimizeUnreachableTerm() {
  std::vector<bool> reachable_term(Size(), false);
  std::vector<bool> visited(Size(), false);

  std::function<void(Node)> check_reachable_term;
  check_reachable_term = [&, this](Node node) {
    if (reachable_term[node]) {
      return;
    }
    visited[node] = true;
    if (IsFinite(node)) {
      reachable_term[node] = true;
      return;
    }

    for (auto& [key, tos] : m_transitions_[node]) {
      for (auto& to : tos) {
        if (!visited[to]) {
          check_reachable_term(to);
        }
        if (reachable_term[to]) {
          reachable_term[node] = true;
          return;
        }
      }
    }
  };

  for (size_t i = 0; i < Size(); ++i) {
    visited.assign(Size(), false);
    check_reachable_term(i);
  }
  for (size_t i = 0; i < Size(); ++i) {
    for (auto& [key, tos] : m_transitions_[i]) {
      std::erase_if(tos, [&](Node node) { return !reachable_term[node]; });
    }
  }
}

}  // namespace rgx

#endif /* REGEX_NFA_HPP */
