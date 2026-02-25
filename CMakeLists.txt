cmake_minimum_required(VERSION 3.16)
project(pplua VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Lua 5.4 ----
# Try pkg-config first, then CMake's FindLua.
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LUA QUIET lua5.4 lua-5.4 lua54 lua)
endif()
if(NOT LUA_FOUND)
    find_package(Lua 5.4 REQUIRED)
    set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
    set(LUA_LIBRARIES    "${LUA_LIBRARIES}")
endif()

# ---- sol2 (header-only) ----
# Set SOL2_INCLUDE_DIR to wherever you've placed sol2's include/.
# e.g.  cmake -DSOL2_INCLUDE_DIR=~/sol2/include ..
set(SOL2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/sol2/include"
    CACHE PATH "Path to sol2 include directory")

if(NOT EXISTS "${SOL2_INCLUDE_DIR}/sol/sol.hpp")
    message(FATAL_ERROR
        "sol2 not found at ${SOL2_INCLUDE_DIR}/sol/sol.hpp  "
        "Set SOL2_INCLUDE_DIR to the correct path.")
endif()

# ---- pplua executable ----
add_executable(pplua
    src/main.cpp
    src/pplua.cpp
    src/lroff.cpp
)

target_include_directories(pplua PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SOL2_INCLUDE_DIR}
    ${LUA_INCLUDE_DIRS}
)

target_link_libraries(pplua PRIVATE ${LUA_LIBRARIES})

target_compile_definitions(pplua PRIVATE
    SOL_ALL_SAFETIES_ON=1
)

# Warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pplua PRIVATE
        -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

install(TARGETS pplua DESTINATION bin)
