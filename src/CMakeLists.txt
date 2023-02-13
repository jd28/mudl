find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(mudl
    main.cpp
    extract.cpp
    model.cpp
    util.cpp
    ModelCache.cpp
    TextureCache.cpp

    bgfx-imgui/imgui_impl_bgfx.cpp
    sdl-imgui/imgui_impl_sdl.cpp

    vs_mudl.sc
    fs_mudl.sc
)

target_link_libraries(mudl PRIVATE
    nw
    bgfx
    bx
    bimg
    imgui::imgui
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    SDL2::SDL2-static
)

target_include_directories(mudl SYSTEM PRIVATE
    ${CMAKE_BINARY_DIR}/include/generated/shaders
    ../external/imgui/imgui
)
