search_unknown_files(CMakeLists.txt
  README.md
  pntr-custom-memory-pool.cpp
  pntr-memory-pool.cpp
  pntr-object-pool.cpp)

add_executable(pntr-custom-memory-pool pntr-custom-memory-pool.cpp)
target_link_libraries(pntr-custom-memory-pool compile_flags pntr)

add_executable(pntr-memory-pool pntr-memory-pool.cpp)
target_link_libraries(pntr-memory-pool compile_flags pntr)

add_executable(pntr-object-pool pntr-object-pool.cpp)
target_link_libraries(pntr-object-pool compile_flags pntr)
