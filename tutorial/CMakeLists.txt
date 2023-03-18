search_unknown_files(CMakeLists.txt
  README.md
  pntr-tutorial.cpp)

add_executable(pntr-tutorial pntr-tutorial.cpp)
target_link_libraries(pntr-tutorial compile_flags pntr)
