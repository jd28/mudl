cmake_minimum_required(VERSION 3.21)
project(mudl VERSION 0.1.0 LANGUAGES CXX C)
set (CMAKE_CXX_STANDARD 20)
include(cmake/Cache.cmake)

# Generate compile_commands.json to make it easier to work with clang based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    add_definitions(
        -D_CRT_SECURE_NO_WARNINGS
        -DUNICODE
        -D_UNICODE
        -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS # abseil
        -DSTBI_WINDOWS_UTF8 # stbi_image
    )
endif()

if(ROLLNW_ENABLE_LEGACY)
add_definitions(-DROLLNW_ENABLE_LEGACY)
endif()

include_directories(SYSTEM external/bgfx/bgfx/include)
include_directories(SYSTEM external/bgfx/bimg/include)
include_directories(SYSTEM external/bgfx/bx/include)
include_directories(SYSTEM external/rollnw/lib)
include_directories(SYSTEM external/rollnw/external)

add_subdirectory(external/bgfx)
add_subdirectory(external/rollnw)

add_subdirectory(src)
