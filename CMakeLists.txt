cmake_minimum_required(VERSION 3.13)

project(codecrafters-git)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

# CONFIGURE_DEPENDS nalo te nava file daalo te build apne aap dubara chale
# (CONFIGURE_DEPENDS re-runs the glob when a file is added or removed)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS src/*.cpp src/*.hpp)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(git ${SOURCE_FILES})

# Headers "utils/error.hpp" vaang paths toh include hunde ne
# (Headers are included by their path from src, e.g. "utils/error.hpp")
target_include_directories(git PRIVATE src)

target_link_libraries(git PRIVATE OpenSSL::Crypto)
target_link_libraries(git PRIVATE ZLIB::ZLIB)
