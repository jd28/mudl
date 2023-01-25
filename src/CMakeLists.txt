find_package(SDL2 CONFIG REQUIRED)

add_executable(mudl
    main.cpp
    model.cpp
    util.cpp

    vs_mudl.sc
    fs_mudl.sc
)

target_link_libraries(mudl
    nw
    bgfx
    bx
    bimg
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    SDL2::SDL2-static
)

target_include_directories(mudl SYSTEM PRIVATE ${CMAKE_BINARY_DIR}/include/generated/shaders)
