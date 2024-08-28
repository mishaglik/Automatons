#include <gtest/gtest.h>

#include "../nfa.hpp"
#include "alphabet.hpp"

using namespace rgx;
using Alphabet = SimpleAlphabet<2>;

TEST(TEST_NFSA, TEST_SETDEL) {
  NFSA<Alphabet> nfsa;
  auto node = nfsa.CreateNode();

  ASSERT_EQ(nfsa.Start() <=> nfsa.Start(), std::partial_ordering::equivalent);
  ASSERT_FALSE(nfsa.Start() < nfsa.Start());
  ASSERT_FALSE(nfsa.Start() > nfsa.Start());
  ASSERT_TRUE(nfsa.Start() <= nfsa.Start());
  ASSERT_TRUE(nfsa.Start() >= nfsa.Start());

  nfsa.AddTransition(nfsa.Start(), 1, node);
  ASSERT_TRUE(nfsa.HasTransition(nfsa.Start(), 1, node));

  nfsa.RemoveTransition(nfsa.Start(), 1, node);
  ASSERT_FALSE(nfsa.HasTransition(nfsa.Start(), 1, node));

  ASSERT_FALSE(nfsa.IsFinite(node));
  nfsa.MakeFinite(node);
  ASSERT_TRUE(nfsa.IsFinite(node));
  nfsa.RemoveFinite(node);
  ASSERT_FALSE(nfsa.IsFinite(node));
  ASSERT_EQ(nfsa.Transitions(nfsa.Start()).at(1).size(), 0);

  nfsa.RemoveEpsilonTransitions();

  std::string ans = "0\n\n\n\n";
  std::stringstream ss;
  nfsa.TextDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST(TEST_NFSA, TEST_START) {
  NFSA<Alphabet> nfsa;
  nfsa.CreateNode();
  nfsa.Validate();
  ASSERT_EQ(nfsa.Start(), 0);
}

TEST(TEST_NFSA, TEST_DUMP) {
  NFSA<Alphabet> nfsa;
  auto node = nfsa.CreateNode();
  nfsa.MakeFinite(node);
  nfsa.AddTransition(nfsa.Start(), 1, node);
  std::string ans = "0\n\n1\n\n0 1 a\n\n";
  std::stringstream ss;
  nfsa.GraphDump("test.png");
  nfsa.TextDump(ss);
  ASSERT_EQ(ss.str(), ans);
  ASSERT_NE(node, nfsa.Start());
}

class TEST_NFAFixtures : public ::testing::Test {
 public:
  void SetUp() override {
    auto node = nfsa1.CreateNode();
    nfsa1.MakeFinite(node);
    nfsa1.AddTransition(nfsa1.Start(), 1, node);

    node = nfsa2.CreateNode();
    nfsa2.MakeFinite(node);
    nfsa2.AddTransition(nfsa2.Start(), 1, node);
  }

  void TearDown() override {
    nfsa1 = NFSA<Alphabet>{};
    nfsa2 = NFSA<Alphabet>{};
  }

  static NFSA<Alphabet> nfsa1;
  static NFSA<Alphabet> nfsa2;

 private:
};

NFSA<Alphabet> TEST_NFAFixtures::nfsa1 = {};
NFSA<Alphabet> TEST_NFAFixtures::nfsa2 = {};

TEST_F(TEST_NFAFixtures, TEST_KLEENE) {
  TEST_NFAFixtures::nfsa1.Kleene();

  std::string ans = "2\n\n1\n2\n\n0 1 a\n1 2 2 0 \n";
  std::stringstream ss;
  TEST_NFAFixtures::nfsa1.TextDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST_F(TEST_NFAFixtures, TEST_OPTIONAL) {
  TEST_NFAFixtures::nfsa1.Optional();

  std::string ans = "2\n\n1\n2\n\n0 1 a\n2 0 \n";
  std::stringstream ss;
  TEST_NFAFixtures::nfsa1.TextDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST_F(TEST_NFAFixtures, TEST_CONCAT) {
  TEST_NFAFixtures::nfsa1.Concat(nfsa2);

  std::string ans = "0\n\n3\n\n0 1 a\n1 2 2 3 a\n\n";
  std::stringstream ss;
  TEST_NFAFixtures::nfsa1.TextDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST_F(TEST_NFAFixtures, TEST_ALTERNATE) {
  TEST_NFAFixtures::nfsa1.Alternate(nfsa2);

  std::string ans = "4\n\n5\n\n0 1 a\n1 5 2 3 a\n3 5 4 0 4 2 \n";
  std::stringstream ss;
  TEST_NFAFixtures::nfsa1.TextDump(ss);
  ASSERT_EQ(ss.str(), ans);
}
