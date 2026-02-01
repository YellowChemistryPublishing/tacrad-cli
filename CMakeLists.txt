cmake_minimum_required(VERSION 3.22)

project(tacrad-cli
    LANGUAGES CXX
    VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(libcxxext/cmake/clang_tidy.cmake)
add_subdirectory(libcxxext)

set(FTXUI_ENABLE_INSTALL OFF CACHE INTERNAL "" FORCE)
add_subdirectory(FTXUI)

set(MINIAUDIO_INSTALL OFF CACHE INTERNAL "" FORCE)
set(MINIAUDIO_NO_EXTRA_NODES ON CACHE INTERNAL "" FORCE)
add_subdirectory(miniaudio)

file(GLOB_RECURSE TACRAD_HEADERS src/*.h)
file(GLOB_RECURSE TACRAD_SRCS src/*.cpp)

add_executable(tacrad ${TACRAD_SRCS})
target_include_directories(tacrad PRIVATE src)
target_precompile_headers(tacrad PRIVATE [[<Preamble.h>]])
target_link_libraries(tacrad PRIVATE sys.BuildSupport.stdc++inc sys sys.Threading screen dom component miniaudio)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(tacrad PRIVATE stdc++exp)
endif()

target_lint_clang_tidy(tacrad "-header-filter=src/.*" ${TACRAD_HEADERS})
