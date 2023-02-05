#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <string_view>

struct Shader {
    // the program ID
    unsigned int id_;

    Shader(std::filesystem::path vertex, std::filesystem::path fragment);

    void use();

    void set_uniform(const std::string& name, bool value) const;
    void set_uniform(const std::string& name, int value) const;
    void set_uniform(const std::string& name, float value) const;
    void set_uniform(const std::string& name, const glm::mat4& value) const;
};
