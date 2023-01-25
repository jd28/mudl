#pragma once

#include <filesystem>

// Gets path to the shaders depending on bgfx backend
std::filesystem::path get_shader_path();
