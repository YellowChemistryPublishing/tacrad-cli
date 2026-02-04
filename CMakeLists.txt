cmake_minimum_required(VERSION 3.22)

project(tacrad-cli
    LANGUAGES CXX
    VERSION 1.0.0
)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Install path prefix." FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(libcxxext/cmake/clang_tidy.cmake)
add_subdirectory(libcxxext EXCLUDE_FROM_ALL)

add_subdirectory(FTXUI EXCLUDE_FROM_ALL)

set(MINIAUDIO_NO_EXTRA_NODES ON CACHE INTERNAL "" FORCE)
add_subdirectory(miniaudio EXCLUDE_FROM_ALL)

set(BUILD_BINDINGS OFF CACHE INTERNAL "" FORCE)
set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
add_subdirectory(taglib EXCLUDE_FROM_ALL)

file(GLOB_RECURSE TACRAD_HEADERS src/*.h)
file(GLOB_RECURSE TACRAD_SRCS src/*.cpp)

add_executable(tacrad ${TACRAD_SRCS})
target_include_directories(tacrad PRIVATE src)
target_precompile_headers(tacrad PRIVATE [[<Preamble.h>]])
target_link_libraries(tacrad PRIVATE
    sys.BuildSupport.CompilerOptions sys.BuildSupport.WarningsAsErrors sys sys.Threading
    screen dom component miniaudio tag
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(tacrad PRIVATE stdc++exp)
endif()

target_lint_clang_tidy(tacrad "-header-filter=src/.*" ${TACRAD_HEADERS})

install(TARGETS tacrad RUNTIME DESTINATION bin)
install(
    DIRECTORY src/exec/
    DESTINATION bin
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
