# Automatons 

## Run
```
cmake . -DCMAKE_BUILD_TYPE=Debug
cmake --build .
make regex_test
./src/regex/regex_test
```

## Sample code 
```C++
#include "regex/tranforms.hpp"
#include "regex/alphabet.hpp"

int main() {
    std::string s1 = "(ab+ba)*(_+a+ba)";
    auto regex = rgx::Regex<rgx::SimpleAlphabet<2>>(s1);
    auto nfa = rgx::NFAFromRegex(rgx);
    nfa.RemoveEpsilonTransitions();
    nfa.GraphDump("NoEps.png");
    auto dfa = rgx::FDFAFromNFA(nfa);
    dfa.GraphDump("dfa.png");
    dfa.Inverse();
    dfa.GraphDump("dfaInverse.png");
    dfa = rgx::Minimize(dfa);
    dfa.GraphDump("MinA.png");
    auto invrgx = rgx::RegexFromFDFA(dfa);
    std::cout << invrgx << '\n';
}

```

## Coverage
```
cmake . -DCMAKE_BUILD_TYPE=Debug
cmake --build .
make rgx_coverage
```

## Task (variant 12)
Function ```MaxRegexMatch```

### Alogorithm
1) Transform regex -> NFSA without epsilon transition (we can do it in polynomial time).
2) Match string in NFSA $O(|Q|^2)$ on one transition, so $O(|Q|^2|S|)$ on all transitions. So it's polynomial time.

Total: polynomial time.




