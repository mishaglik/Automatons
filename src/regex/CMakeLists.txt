
add_library(rgx INTERFACE regex.hpp tokenizer.hpp nfa.hpp fdfa.hpp tranforms.hpp)
target_include_directories(rgx INTERFACE .)



file(GLOB TESTS_SRC
    "./test/*.hpp"
    "./test/*.cpp")
add_executable(regex_test ${TESTS_SRC})
target_link_libraries(regex_test rgx ${GTEST_LIBRARIES})
setup_target_for_coverage(rgx_coverage regex_test coverage)
SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")
SET(CMAKE_C_FLAGS   "-g -O0 -fprofile-arcs -ftest-coverage --coverage -fno-inline -fno-inline-small-functions -fno-default-inline")