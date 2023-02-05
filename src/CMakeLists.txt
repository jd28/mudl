find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(mudl
    main.cpp
    model.cpp
    shader.cpp
    util.cpp
    ModelCache.cpp
    TextureCache.cpp
)

target_link_libraries(mudl PRIVATE
    nw
    imgui::imgui
    glfw
    glad::glad
)

target_include_directories(mudl SYSTEM PRIVATE
    ${CMAKE_BINARY_DIR}/include/generated/shaders
    ../external/imgui/imgui
)
