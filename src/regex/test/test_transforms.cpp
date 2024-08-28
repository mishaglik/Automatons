#include <gtest/gtest.h>

#include "../alphabet.hpp"
#include "../tranforms.hpp"

TEST(TRANSFORM_TEST, RECREATION) {
  std::string s1 = "(ab+ba)*(_+a+ba)";
  auto regex = rgx::Regex<rgx::SimpleAlphabet<2>>(s1);
  auto nfa = rgx::NFAFromRegex(regex);
  nfa.RemoveEpsilonTransitions();
  nfa.GraphDump("NoEps.png");
  auto dfa = rgx::FDFAFromNFA(nfa);
  dfa.GraphDump("dfa.png");
  dfa.Inverse();
  dfa.GraphDump("dfaInverse.png");
  dfa = rgx::Minimize(dfa);
  dfa.GraphDump("MinA.png");
  auto invrgx = rgx::RegexFromFDFA(dfa);
  std::stringstream ss;
  ss << invrgx;
  ASSERT_EQ(ss.str(), "(ba+ab)*((aa+bb)(a+b)*+b)");
}