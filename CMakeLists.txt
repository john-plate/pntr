option(PNTR_SINGLE_HEADER "Enable single header generation" OFF)
option(PNTR_TUTORIAL "Enable tutorial" OFF)
option(PNTR_TESTING "Enable testing" OFF)

if(PNTR_TESTING)
  cmake_minimum_required(VERSION 3.24...3.25.2)
else()
  cmake_minimum_required(VERSION 3.20...3.25.2)
endif()

set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "Configurations")
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")

if(PNTR_SINGLE_HEADER OR PNTR_TUTORIAL OR PNTR_TESTING)
  set(languages CXX)
endif()

project(pntr
  VERSION 0.1
  DESCRIPTION "Intrusive Shared Pointer for C++17"
  LANGUAGES ${languages})

include(cmake/Tools.cmake)

# Set default compile flags and create interface library with project-specific compile flags
message(VERBOSE "CMAKE_CXX_COMPILER_ID = '${CMAKE_CXX_COMPILER_ID}'")
add_library(compile_flags INTERFACE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND WIN32))
  add_compile_options(/permissive-)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(/EHsc) # missing in x86 config
    target_compile_options(compile_flags
      INTERFACE
        -Wall -Wextra -Werror
        -Wno-c++98-compat -Wno-c++98-compat-pedantic
        -Wno-disabled-macro-expansion -Wno-exit-time-destructors)
  else()
    target_compile_options(compile_flags INTERFACE /bigobj /W4 /WX /wd4324)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options($<$<CONFIG:Debug>:-Og>)
  add_link_options(-static-libgcc -static-libstdc++ $<$<CONFIG:Release>:-s>)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(compile_flags
      INTERFACE
        -Werror -Wfatal-errors -Wall -Wextra -pedantic -pedantic-errors
        -Wcast-align -Wcast-qual -Wdisabled-optimization -Wextra-semi -Wformat=2 -Winit-self
        -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wnon-virtual-dtor -Wold-style-cast
        -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel
        -Wstrict-overflow=5 -Wundef)
  else()
    target_compile_options(compile_flags
      INTERFACE
        -Wall -Wextra -Werror
        -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wextra-semi -Wextra-semi-stmt)
  endif()
endif()

# Define custom variables for project package
set(project_export ${PROJECT_NAME}Export)
set(import_path lib/cmake/${PROJECT_NAME})
set(import_filename ${PROJECT_NAME}.cmake)
set(config_filename ${PROJECT_NAME}Config.cmake)

# Add subdirectory for project package
add_subdirectory(pntr)

# Install package files
export(EXPORT ${project_export}
  FILE ${import_path}/${import_filename})
install(EXPORT ${project_export}
  DESTINATION ${import_path}
  FILE ${import_filename})

# Generate and install package config file
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/${config_filename}.in
  ${import_path}/${config_filename}
  INSTALL_DESTINATION ${import_path})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${import_path}/${config_filename}
  DESTINATION ${import_path})

# Install the license file
install(FILES LICENSE DESTINATION include/pntr)

# Add subdirectories for tests and tutorial
if(PNTR_SINGLE_HEADER)
  add_subdirectory(single-header)
endif()
if(PNTR_TUTORIAL)
  add_subdirectory(tutorial)
  add_subdirectory(examples)
endif()
if(PNTR_TESTING)
  include(CTest)
  add_subdirectory(tests)
endif()
