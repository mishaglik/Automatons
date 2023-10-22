#include <gtest/gtest.h>

#include "../regex.hpp"
#include "alphabet.hpp"

TEST(REGEX_TEST, RECREATION) {
  rgx::CharAlphabet a{};
  (void)a;

  std::string rgx = "a+_((b?aosg3\\\\\\?)?+\\+\\*a+_)*+((a+b)(c+d))?";
  auto regex = rgx::Regex<rgx::CharAlphabet>(rgx);
  ASSERT_NE(regex.GetImpl(), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CharAlphabet>::FromString("ab++"), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CharAlphabet>::FromString("a\\"), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CharAlphabet>::FromString("a+("), nullptr);

  std::stringstream ss;
  ss << regex;
  ASSERT_EQ(ss.str(), rgx);
  const rgx::RegexImpl<rgx::CharAlphabet>* r =
      rgx::RegexImpl<rgx::CharAlphabet>::FromString("(a+bb*(ab+b))?");
  auto regex_copy = regex;
  regex_copy.Optional();
  ASSERT_NE(regex_copy, regex);
  ss << *mgk::Cast<rgx::RegexQuantified<rgx::CharAlphabet>>(r);
}

TEST(REGEX_TEST, RECREATION2) {
  rgx::SimpleAlphabet<2> alph{};
  (void)alph;

  std::string rgx = "a+_((b?aaaa)?+a+_)*+((a+b)(a+b))?";
  auto regex = rgx::Regex<rgx::SimpleAlphabet<2>>(rgx);
  ASSERT_NE(regex.GetImpl(), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("ab++"),
            nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("a\\"), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("a+("), nullptr);

  std::stringstream ss;
  ss << regex;
  ASSERT_EQ(ss.str(), rgx);

  const rgx::RegexImpl<rgx::SimpleAlphabet<2>>* r =
      rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("(a+bb*(ab+b))?");
  ss << *mgk::Cast<rgx::RegexQuantified<rgx::SimpleAlphabet<2>>>(r);
  r = rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("a+b?");
  ss << *mgk::Cast<rgx::RegexAlternate<rgx::SimpleAlphabet<2>>>(r);
  ;
  r = rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("a");
  ss << *mgk::Cast<rgx::RegexLetter<rgx::SimpleAlphabet<2>>>(r);
  ;
}

TEST(REGEX_TEST, RECREATION3) {
  rgx::CanonicalAlphabet<3> alph{};
  (void)alph;

  std::string rgx = "a+1((b?aaaa)?+a+1)*+((a+b)(a+b))?";
  auto regex = rgx::Regex<rgx::CanonicalAlphabet<3>>(rgx);
  ASSERT_NE(regex.GetImpl(), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CanonicalAlphabet<3>>::FromString("ab++"),
            nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CanonicalAlphabet<3>>::FromString("a\\"), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CanonicalAlphabet<3>>::FromString("a+("), nullptr);

  std::stringstream ss;
  ss << regex;
  ASSERT_EQ(ss.str(), rgx);

  const rgx::RegexImpl<rgx::CanonicalAlphabet<3>>* r =
      rgx::RegexImpl<rgx::CanonicalAlphabet<3>>::FromString("(a+bb*(ab+b))?");
  ss << *mgk::Cast<rgx::RegexQuantified<rgx::CanonicalAlphabet<3>>>(r);
  r = rgx::RegexImpl<rgx::CanonicalAlphabet<3>>::FromString("a+b?");
  ss << *mgk::Cast<rgx::RegexAlternate<rgx::CanonicalAlphabet<3>>>(r);
  r = rgx::RegexImpl<rgx::CanonicalAlphabet<3>>::FromString("a");
  ss << *mgk::Cast<rgx::RegexLetter<rgx::CanonicalAlphabet<3>>>(r);

  auto r1 = regex.Kleene().Optional().Reverse();

}
