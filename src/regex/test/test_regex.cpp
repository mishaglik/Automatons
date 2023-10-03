#include <gtest/gtest.h>

#include "../regex.hpp"
#include "alphabet.hpp"

TEST(REGEX_TEST, RECREATION) {
  rgx::CharAlphabet a{};
  (void)a;

  std::string rgx = "a+_((b?aosg3\\\\\\?)?+\\+\\*a+_)*+((a+b)(c+d))?";
  auto* regex = rgx::RegexImpl<rgx::CharAlphabet>::FromString(rgx);
  ASSERT_NE(regex, nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CharAlphabet>::FromString("ab++"), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CharAlphabet>::FromString("a\\"), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::CharAlphabet>::FromString("a+("), nullptr);

  std::stringstream ss;
  ss << *regex;
  ASSERT_EQ(ss.str(), rgx);
  const rgx::RegexImpl<rgx::CharAlphabet>* r = rgx::RegexImpl<rgx::CharAlphabet>::FromString("(a+bb*(ab+b))?");
  ss << *mgk::cast<rgx::RegexQuantified<rgx::CharAlphabet>>(r);
  delete regex;
}

TEST(REGEX_TEST, RECREATION2) {
  rgx::SimpleAlphabet<2> alph{};
  (void)alph;

  std::string rgx = "a+_((b?aaaa)?+a+_)*+((a+b)(a+b))?";
  auto* regex = rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString(rgx);
  ASSERT_NE(regex, nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("ab++"), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("a\\"), nullptr);
  ASSERT_EQ(rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("a+("), nullptr);

  std::stringstream ss;
  ss << *regex;
  ASSERT_EQ(ss.str(), rgx);
  delete regex;

  const rgx::RegexImpl<rgx::SimpleAlphabet<2>>* r =
      rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("(a+bb*(ab+b))?");
  ss << *mgk::cast<rgx::RegexQuantified<rgx::SimpleAlphabet<2>>>(r);
  r = rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("a+b?");
  ss << *mgk::cast<rgx::RegexAlternate<rgx::SimpleAlphabet<2>>>(r);;
  r = rgx::RegexImpl<rgx::SimpleAlphabet<2>>::FromString("a");
  ss << *mgk::cast<rgx::RegexLetter<rgx::SimpleAlphabet<2>>>(r);;
}

// TEST(REGEX_TEST, RECREATION3) {

//     rgx::SimpleAlphabet<3> alph{};
//     (void) alph;

//     std::string rgx = "a+_((b?aaaa)?+a+_)*+((a+b)(c+b))?";
//     auto* regex = rgx::Regex<rgx::SimpleAlphabet<3>>::FromString(rgx);
//     ASSERT_NE(regex, nullptr);
//     ASSERT_EQ(rgx::Regex<rgx::SimpleAlphabet<3>>::FromString("ab++"),
//     nullptr);
//     ASSERT_EQ(rgx::Regex<rgx::SimpleAlphabet<3>>::FromString("a\\"),
//     nullptr);
//     ASSERT_EQ(rgx::Regex<rgx::SimpleAlphabet<3>>::FromString("a+("),
//     nullptr);

//     std::stringstream ss;
//     ss << *regex;
//     ASSERT_EQ(ss.str(), rgx);
//     delete regex;
// }

// TEST(REGEX_TEST, RECREATION4) {

//     rgx::SimpleAlphabet<4> alph{};
//     (void) alph;

//     std::string rgx = "a+_((b?aad)?+a+_)*+((a+b)(a+b))?";
//     auto* regex = rgx::Regex<rgx::SimpleAlphabet<4>>::FromString(rgx);
//     ASSERT_NE(regex, nullptr);
//     ASSERT_EQ(rgx::Regex<rgx::SimpleAlphabet<4>>::FromString("ab++"),
//     nullptr);
//     ASSERT_EQ(rgx::Regex<rgx::SimpleAlphabet<4>>::FromString("a\\"),
//     nullptr);
//     ASSERT_EQ(rgx::Regex<rgx::SimpleAlphabet<4>>::FromString("a+("),
//     nullptr);

//     std::stringstream ss;
//     ss << *regex;
//     ASSERT_EQ(ss.str(), rgx);
//     delete regex;
// }