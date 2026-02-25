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

add_subdirectory(libcxxext EXCLUDE_FROM_ALL)
add_subdirectory(FTXUI EXCLUDE_FROM_ALL)

set(MINIAUDIO_NO_EXTRA_NODES ON CACHE INTERNAL "" FORCE)
add_subdirectory(miniaudio EXCLUDE_FROM_ALL)

set(BUILD_BINDINGS OFF CACHE INTERNAL "" FORCE)
set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
add_subdirectory(taglib EXCLUDE_FROM_ALL)

add_library(taglib INTERFACE)
target_compile_options(taglib INTERFACE $<$<CXX_COMPILER_ID:GNU>:-Wno-sign-compare> $<$<CXX_COMPILER_ID:GNU>:-Wno-stringop-overflow>) # Spurious. Happens when optimizations are enabled.
target_include_directories(taglib INTERFACE
    taglib taglib/taglib taglib/taglib/toolkit taglib/taglib/mpeg/id3v2 taglib/taglib/flac
    ${CMAKE_BINARY_DIR}/tag/include
)
target_link_libraries(taglib INTERFACE tag)

file(GLOB_RECURSE TACRAD_HEADERS src/*.h)
file(GLOB_RECURSE TACRAD_SRCS src/*.cpp)

add_executable(tacrad ${TACRAD_SRCS})
target_include_directories(tacrad PRIVATE src)
target_precompile_headers(tacrad PRIVATE
    [[<ftxui/component/animation.hpp>]]
    [[<ftxui/component/captured_mouse.hpp>]]
    [[<ftxui/component/component.hpp>]]
    [[<ftxui/component/component_base.hpp>]]
    [[<ftxui/component/component_options.hpp>]]
    [[<ftxui/component/event.hpp>]]
    [[<ftxui/component/loop.hpp>]]
    [[<ftxui/component/mouse.hpp>]]
    [[<ftxui/component/receiver.hpp>]]
    [[<ftxui/component/screen_interactive.hpp>]]
    [[<ftxui/component/task.hpp>]]
    [[<ftxui/dom/canvas.hpp>]]
    [[<ftxui/dom/deprecated.hpp>]]
    [[<ftxui/dom/direction.hpp>]]
    [[<ftxui/dom/elements.hpp>]]
    [[<ftxui/dom/flexbox_config.hpp>]]
    [[<ftxui/dom/linear_gradient.hpp>]]
    [[<ftxui/dom/node.hpp>]]
    [[<ftxui/dom/requirement.hpp>]]
    [[<ftxui/dom/selection.hpp>]]
    [[<ftxui/dom/table.hpp>]]
    [[<ftxui/dom/take_any_args.hpp>]]
    [[<ftxui/screen/box.hpp>]]
    [[<ftxui/screen/color.hpp>]]
    [[<ftxui/screen/color_info.hpp>]]
    [[<ftxui/screen/deprecated.hpp>]]
    [[<ftxui/screen/image.hpp>]]
    [[<ftxui/screen/pixel.hpp>]]
    [[<ftxui/screen/screen.hpp>]]
    [[<ftxui/screen/string.hpp>]]
    [[<ftxui/screen/terminal.hpp>]]
    [[<ftxui/util/autoreset.hpp>]]
    [[<ftxui/util/ref.hpp>]]
)
target_link_libraries(tacrad PRIVATE
    sys.BuildSupport.CompilerOptions sys.BuildSupport.WarningsAsErrors sys.BuildSupport.stdc++inc
    sys sys.Text
    screen dom component miniaudio taglib
    $<$<AND:$<PLATFORM_ID:Linux>,$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>>:atomic>
)

target_lint_clang_tidy(tacrad "-header-filter=src/.*" ${TACRAD_HEADERS})

install(TARGETS tacrad RUNTIME DESTINATION bin)
install(
    DIRECTORY src/exec/
    DESTINATION bin
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_IGNORE_FILES "/[^/]*build/" "/checks.json" "/Doxyfile" "/workflows/" "libcxxext/sys.Text/data_unicode/" "/\\\\.[^/]+/")
#                                            ^ Library consumers don't need.                                             ^ Dirs beginning with a dot, i.e. `.cache/`.
include(CPack)
