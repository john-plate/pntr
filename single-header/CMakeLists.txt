search_unknown_files_recurse(CMakeLists.txt
  README.md
  pntr-single-header-generator.cpp
  pntr/pntr.hpp)

add_executable(pntr-single-header-generator pntr-single-header-generator.cpp)
target_link_libraries(pntr-single-header-generator compile_flags)
target_compile_features(pntr-single-header-generator PRIVATE cxx_std_17)

get_target_property(pntr_headers_abs pntr HEADER_SET)
foreach(header ${pntr_headers_abs})
  cmake_path(RELATIVE_PATH header BASE_DIRECTORY ${pntr_SOURCE_DIR})
  list(APPEND pntr_headers_rel ${header})
endforeach()

set(single_header pntr/pntr.hpp)
set(single_header_bin ${CMAKE_CURRENT_BINARY_DIR}/${single_header})
set(single_header_src ${CMAKE_CURRENT_SOURCE_DIR}/${single_header})

add_custom_command(OUTPUT ${single_header}
  COMMAND pntr-single-header-generator ${single_header_bin} LICENSE ${pntr_headers_rel}
  COMMAND ${CMAKE_COMMAND} -E compare_files ${single_header_bin} ${single_header_src} || ${CMAKE_COMMAND} -E copy ${single_header_bin} ${single_header_src}
  DEPENDS pntr-single-header-generator ${pntr_SOURCE_DIR}/LICENSE ${pntr_headers_abs}
  WORKING_DIRECTORY ${pntr_SOURCE_DIR})

add_library(pntr-single-header INTERFACE)
target_sources(pntr-single-header
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}
    FILES ${single_header_bin})
target_compile_features(pntr-single-header INTERFACE cxx_std_17)
