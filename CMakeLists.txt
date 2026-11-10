# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(cminux VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# --- cminux runtime library ------------------------------------------------
add_library(cminux_outfn STATIC
    lib/src/outfn.cpp
)
target_include_directories(cminux_outfn PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# --- cminux compiler -------------------------------------------------------
add_executable(cminux
    src/main.cpp
    src/tokenizer.cpp
    src/rewriter.cpp
    src/emitter.cpp
    src/compiler.cpp
    src/driver.cpp
)
target_link_libraries(cminux PRIVATE cminux_outfn)

# Bake the source-tree path of outfn.hpp into the binary so it works
# without an install step. In an installed build this would be set to the
# install prefix by CMake's configure_package_config_file.
target_compile_definitions(cminux PRIVATE
    CMX_LIB_DIR_DEFAULT="${CMAKE_CURRENT_SOURCE_DIR}/lib"
)