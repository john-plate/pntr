include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(Catch2
  GIT_REPOSITORY     https://github.com/catchorg/Catch2.git
  GIT_TAG            v3.3.1
  GIT_SHALLOW        TRUE
  GIT_PROGRESS       TRUE
  FIND_PACKAGE_ARGS  3)
FetchContent_MakeAvailable(Catch2)

set(pntr_tests_sources
  tests-common.hpp
  tests-Counter.cpp
  tests-ControlData.cpp
  tests-ControlNew.cpp
  tests-ControlAlloc.cpp
  tests-SharedPtr.cpp
  tests-WeakPtr.cpp)

set(pntr_benchmark_sources
  benchmark-Counter.cpp)

search_unknown_files(CMakeLists.txt
  README.md
  ${pntr_tests_sources}
  ${pntr_benchmark_sources})

add_executable(pntr_tests ${pntr_tests_sources})
target_link_libraries(pntr_tests compile_flags pntr Catch2::Catch2WithMain)

if(PNTR_SINGLE_HEADER)
  add_executable(pntr_single_header_tests ${pntr_tests_sources})
  target_compile_definitions(pntr_single_header_tests PRIVATE TEST_PREFIX="Single header - ")
  target_link_libraries(pntr_single_header_tests compile_flags pntr-single-header Catch2::Catch2WithMain)
endif()

add_executable(pntr_benchmark ${pntr_benchmark_sources})
target_link_libraries(pntr_benchmark compile_flags pntr Catch2::Catch2WithMain)

include(Catch)
catch_discover_tests(pntr_tests)
if(PNTR_SINGLE_HEADER)
  catch_discover_tests(pntr_single_header_tests)
endif()
