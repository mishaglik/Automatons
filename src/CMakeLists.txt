add_subdirectory(regex)

add_executable(main main.cpp)
target_link_libraries(main rgx)
