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
    nfa.removeEpsilonTransitions();
    nfa.graphDump("NoEps.png");
    auto dfa = rgx::FDFAFromNFA(nfa);
    dfa.graphDump("dfa.png");
    dfa.inverse();
    dfa.graphDump("dfaInverse.png");
    dfa = rgx::Minimize(dfa);
    dfa.graphDump("MinA.png");
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