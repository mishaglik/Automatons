#include <gtest/gtest.h>

#include "../alphabet.hpp"

TEST(AlphabetTest, ALPH_CHR_TEST) {
  ASSERT_EQ(rgx::SimpleAlphabet<2>::Chr(1), 'a');
  ASSERT_EQ(rgx::SimpleAlphabet<2>::Chr(0), 0);
}

TEST(AlphabetTest, ALPH_ORD_TEST) {
  ASSERT_EQ(rgx::SimpleAlphabet<2>::Ord(0), 0);
  ASSERT_EQ(rgx::SimpleAlphabet<2>::Ord('a'), 1);
}

TEST(AlphabetTest, ALPH_ISSPACE_TEST) {
  ASSERT_TRUE(rgx::SimpleAlphabet<2>::IsSpace(' '));
  ASSERT_FALSE(rgx::SimpleAlphabet<2>::IsSpace('a'));
}

TEST(AlphabetTest, ALPH_ESCAPE_TEST) {
  ASSERT_TRUE(rgx::SimpleAlphabet<2>::NeedEscape('\\'));
  ASSERT_TRUE(rgx::SimpleAlphabet<2>::NeedEscape('?'));
  ASSERT_TRUE(rgx::SimpleAlphabet<2>::NeedEscape('*'));
  ASSERT_TRUE(rgx::SimpleAlphabet<2>::NeedEscape('+'));
  ASSERT_TRUE(rgx::SimpleAlphabet<2>::NeedEscape(')'));
  ASSERT_TRUE(rgx::SimpleAlphabet<2>::NeedEscape('('));
  ASSERT_TRUE(rgx::SimpleAlphabet<2>::NeedEscape('_'));
}

TEST(AlphabetChrTest, ALPH_CHR_TEST) {
  ASSERT_EQ(rgx::CharAlphabet::Chr('a'), 'a');
  ASSERT_EQ(rgx::CharAlphabet::Chr(0), 0);
}

TEST(AlphabetChrTest, ALPH_ORD_TEST) {
  ASSERT_EQ(rgx::CharAlphabet::Ord(0), 0);
  ASSERT_EQ(rgx::CharAlphabet::Ord('a'), 'a');
}

TEST(AlphabetChrTest, ALPH_ISSPACE_TEST) {
  ASSERT_TRUE(rgx::CharAlphabet::IsSpace(' '));
  ASSERT_FALSE(rgx::CharAlphabet::IsSpace('a'));
}

TEST(AlphabetChrTest, ALPH_ESCAPE_TEST) {
  ASSERT_TRUE(rgx::CharAlphabet::NeedEscape('\\'));
  ASSERT_TRUE(rgx::CharAlphabet::NeedEscape('?'));
  ASSERT_TRUE(rgx::CharAlphabet::NeedEscape('*'));
  ASSERT_TRUE(rgx::CharAlphabet::NeedEscape('+'));
  ASSERT_TRUE(rgx::CharAlphabet::NeedEscape(')'));
  ASSERT_TRUE(rgx::CharAlphabet::NeedEscape('('));
  ASSERT_TRUE(rgx::CharAlphabet::NeedEscape('_'));
}