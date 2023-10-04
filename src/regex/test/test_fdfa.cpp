#include <gtest/gtest.h>

#include "../fdfa.hpp"
#include "alphabet.hpp"

using namespace rgx;
using Alphabet = SimpleAlphabet<2>;

TEST(TEST_FDFA, TEST_SETDEL) {
  FDFA<Alphabet> fdfa;
  fdfa.create_node();

  fdfa.set_transition(0, 0, 1);
  ASSERT_TRUE(fdfa.has_transition(0, 0, 1));

  fdfa.remove_transition(0, 0);
  ASSERT_FALSE(fdfa.has_transition(0, 0, 1));

  ASSERT_FALSE(fdfa.isFinite(1));
  fdfa.makeFinite(1);
  ASSERT_TRUE(fdfa.isFinite(1));
  fdfa.removeFinite(1);
  ASSERT_FALSE(fdfa.isFinite(1));

  std::array<uint64_t, 3> trans = {{-1, -1, -1}};

  ASSERT_EQ(fdfa.transitions(0), trans);
  ASSERT_EQ(fdfa.transitions(1), trans);
}

TEST(TEST_FDFA, TEST_START) {
  FDFA<Alphabet> fdfa;
  fdfa.create_node();
  ASSERT_EQ(fdfa.start(), 0);
}

TEST(TEST_FDFA, TEST_DUMP) {
  FDFA<Alphabet> fdfa;
  fdfa.create_node();
  fdfa.makeFinite(1);
  fdfa.set_transition(0, 1, 1);
  std::string ans = "0\n\n1\n\n0 1 a\n\n";
  std::stringstream ss;
  fdfa.graphDump("test.png");
  fdfa.textDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST(TEST_FDFA, TEST_INVERSE) {
  FDFA<Alphabet> fdfa;
  fdfa.create_node();
  fdfa.makeFinite(1);
  fdfa.set_transition(0, 1, 1);
  auto fdfa2 = fdfa;
  fdfa2.inverse();
  fdfa2.inverse();
  std::stringstream ss1;
  fdfa.textDump(ss1);
  std::stringstream ss2;
  fdfa2.textDump(ss2);
  ASSERT_EQ(ss1.str(), ss2.str());
}