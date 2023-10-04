#include <gtest/gtest.h>

#include "../nfa.hpp"
#include "alphabet.hpp"

using namespace rgx;
using Alphabet = SimpleAlphabet<2>;

TEST(TEST_NFSA, TEST_SETDEL) {
  NFSA<Alphabet> nfsa;
  auto node = nfsa.create_node();

  ASSERT_EQ(nfsa.start() <=> nfsa.start(), std::partial_ordering::equivalent);
  ASSERT_FALSE(nfsa.start() < nfsa.start());
  ASSERT_FALSE(nfsa.start() > nfsa.start());
  ASSERT_TRUE(nfsa.start() <= nfsa.start());
  ASSERT_TRUE(nfsa.start() >= nfsa.start());

  nfsa.add_transition(nfsa.start(), 1, node);
  ASSERT_TRUE(nfsa.has_transition(nfsa.start(), 1, node));

  nfsa.remove_transition(nfsa.start(), 1, node);
  ASSERT_FALSE(nfsa.has_transition(nfsa.start(), 1, node));

  ASSERT_FALSE(nfsa.isFinite(node));
  nfsa.makeFinite(node);
  ASSERT_TRUE(nfsa.isFinite(node));
  nfsa.removeFinite(node);
  ASSERT_FALSE(nfsa.isFinite(node));
  ASSERT_EQ(nfsa.transitions(nfsa.start()).at(1).size(), 0);

  nfsa.removeEpsilonTransitions();

  std::string ans = "0\n\n\n\n";
  std::stringstream ss;
  nfsa.textDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST(TEST_NFSA, TEST_START) {
  NFSA<Alphabet> nfsa;
  nfsa.create_node();
  nfsa.validate();
  ASSERT_EQ(nfsa.start(), 0);
}

TEST(TEST_NFSA, TEST_DUMP) {
  NFSA<Alphabet> nfsa;
  auto node = nfsa.create_node();
  nfsa.makeFinite(node);
  nfsa.add_transition(nfsa.start(), 1, node);
  std::string ans = "0\n\n1\n\n0 1 a\n\n";
  std::stringstream ss;
  nfsa.graphDump("test.png");
  nfsa.textDump(ss);
  ASSERT_EQ(ss.str(), ans);
  ASSERT_NE(node, nfsa.start());
}

class TEST_NFAFixtures : public ::testing::Test {
 public:
  void SetUp() override {
    auto node = nfsa1.create_node();
    nfsa1.makeFinite(node);
    nfsa1.add_transition(nfsa1.start(), 1, node);

    node = nfsa2.create_node();
    nfsa2.makeFinite(node);
    nfsa2.add_transition(nfsa2.start(), 1, node);
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
  TEST_NFAFixtures::nfsa1.kleene();

  std::string ans = "2\n\n1\n2\n\n0 1 a\n1 2 2 0 \n";
  std::stringstream ss;
  TEST_NFAFixtures::nfsa1.textDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST_F(TEST_NFAFixtures, TEST_OPTIONAL) {
  TEST_NFAFixtures::nfsa1.optional();

  std::string ans = "2\n\n1\n2\n\n0 1 a\n2 0 \n";
  std::stringstream ss;
  TEST_NFAFixtures::nfsa1.textDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST_F(TEST_NFAFixtures, TEST_CONCAT) {
  TEST_NFAFixtures::nfsa1.concat(nfsa2);

  std::string ans = "0\n\n3\n\n0 1 a\n1 2 2 3 a\n\n";
  std::stringstream ss;
  TEST_NFAFixtures::nfsa1.textDump(ss);
  ASSERT_EQ(ss.str(), ans);
}

TEST_F(TEST_NFAFixtures, TEST_ALTERNATE) {
  TEST_NFAFixtures::nfsa1.alternate(nfsa2);

  std::string ans = "4\n\n5\n\n0 1 a\n1 5 2 3 a\n3 5 4 0 4 2 \n";
  std::stringstream ss;
  TEST_NFAFixtures::nfsa1.textDump(ss);
  ASSERT_EQ(ss.str(), ans);
}
