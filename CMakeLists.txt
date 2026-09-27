cmake_minimum_required(VERSION 3.13)

project(codecrafters-git)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

# CONFIGURE_DEPENDS nalo te nava file daalo te build apne aap dubara chale
# (CONFIGURE_DEPENDS re-runs the glob when a file is added or removed)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
list(REMOVE_ITEM SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# Everything except main() goes in a library so the pack reader test can drive
# the real code instead of a copy of it. The `git` target is unchanged: it is
# still main() plus all of src.
add_library(gitcore STATIC ${SOURCE_FILES})
# Headers "utils/error.hpp" vaang paths toh include hunde ne
# (Headers are included by their path from src, e.g. "utils/error.hpp")
target_include_directories(gitcore PUBLIC src)
target_link_libraries(gitcore PUBLIC OpenSSL::Crypto ZLIB::ZLIB)

add_executable(git src/main.cpp)
target_link_libraries(git PRIVATE gitcore)

# ---- tests -----------------------------------------------------------------
# tests/run.sh is the entrypoint and does the build itself; these targets exist
# so `ctest` works for anyone who expects it to.
# See tests/README.md for what the suites do and do not cover.
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_executable(pack_reader_test tests/unit/pack_reader_test.cpp)
target_link_libraries(pack_reader_test PRIVATE gitcore)

add_test(NAME pack_reader
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/unit/run_pack_test.py
                 $<TARGET_FILE:pack_reader_test>)

add_test(NAME clone_trunk
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/clone_trunk_test.sh
                 $<TARGET_FILE:git>)

add_test(NAME suite_size
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_size.py
                 ${CMAKE_CURRENT_SOURCE_DIR}/tests)
