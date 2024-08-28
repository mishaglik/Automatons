cmake_minimum_required(VERSION 3.20)

project(automatons VERSION 0.1.0 LANGUAGES CXX)

include(cmake/Logging.cmake)
include(cmake/Ccache.cmake)
include(cmake/Helpers.cmake)
include(cmake/CheckCompiler.cmake)

include(cmake/CompileOptions.cmake)
# include(cmake/Sanitize.cmake)
include(cmake/Development.cmake)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)
include(cmake/CodeCoverage.cmake)
include_directories(${GTEST_INCLUDE_DIRS})

# Must be after lib not to affect them.
# include(cmake/Werror.cmake)

add_subdirectory(src)
