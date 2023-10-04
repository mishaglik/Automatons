#include <gtest/gtest.h>

#include "../tranforms.hpp"
#include "../alphabet.hpp"

TEST(TRANSFORM_TEST, RECREATION) {
    std::string s1 = "(ab+ba)*(_+a+ba)";
    auto regex = rgx::Regex<rgx::SimpleAlphabet<2>>(s1);
    auto nfa = rgx::NFAFromRegex(regex);
    nfa.removeEpsilonTransitions();
    nfa.graphDump("NoEps.png");
    auto dfa = rgx::FDFAFromNFA(nfa);
    dfa.graphDump("dfa.png");
    dfa.inverse();
    dfa.graphDump("dfaInverse.png");
    dfa = rgx::Minimize(dfa);
    dfa.graphDump("MinA.png");
    auto invrgx = rgx::RegexFromFDFA(dfa);
    std::stringstream ss;
    ss << invrgx;
    ASSERT_EQ(ss.str(), "(ba+ab)*((aa+bb)(a+b)*+b)");
}