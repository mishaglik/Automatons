#include <gtest/gtest.h>

#include "../fdfa.hpp"
#include "alphabet.hpp"

using namespace rgx;
using Alphabet = SimpleAlphabet<2>;

TEST(TEST_FDFA, TEST_SETDEL) {
  FDFA<Alphabet> fdfa;
  fdfa.CreateNode();

  fdfa.SetTransition(0, 0, 1);
  ASSERT_TRUE(fdfa.HasTransition(0, 0, 1));

  fdfa.RemoveTransition(0, 0);
  ASSERT_FALSE(fdfa.HasTransition(0, 0, 1));

  ASSERT_FALSE(fdfa.IsFinite(1));
  fdfa.MakeFinite(1);
  ASSERT_TRUE(fdfa.IsFinite(1));
  fdfa.RemoveFinite(1);
  ASSERT_FALSE(fdfa.IsFinite(1));

  std::array<uint64_t, 3> trans = {{-1, -1, -1}};

  ASSERT_EQ(fdfa.Transitions(0), trans);
  ASSERT_EQ(fdfa.Transitions(1), trans);
}

TEST(TEST_FDFA, TEST_START) {
  FDFA<Alphabet> fdfa;
  fdfa.CreateNode();
  ASSERT_EQ(fdfa.Start(), 0);
}

TEST(TEST_FDFA, TEST_DUMP) {
  FDFA<Alphabet> fdfa;
  fdfa.CreateNode();
  fdfa.MakeFinite(1);
  fdfa.SetTransition(0, 1, 1);
  std::string ans = "0\n\n1\n\n0 1 a\n\n";
  std::stringstream ss;
  fdfa.GraphDump("test.png");
  fdfa.TextDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST(TEST_FDFA, TEST_INVERSE) {
  FDFA<Alphabet> fdfa;
  fdfa.CreateNode();
  fdfa.MakeFinite(1);
  fdfa.SetTransition(0, 1, 1);
  auto fdfa2 = fdfa;
  fdfa2.Inverse();
  fdfa2.Inverse();
  std::stringstream ss1;
  fdfa.TextDump(ss1);
  std::stringstream ss2;
  fdfa2.TextDump(ss2);
  ASSERT_EQ(ss1.str(), ss2.str());
}