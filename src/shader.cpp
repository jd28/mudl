#include "shader.hpp"

#include <nw/log.hpp>
#include <nw/util/ByteArray.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>

Shader::Shader(std::filesystem::path vertex, std::filesystem::path fragment)
{
    unsigned int vs, fs;
    int success;
    char infoLog[512];

    auto vs_bytes = nw::ByteArray::from_file(vertex);
    auto vs_string = std::string(vs_bytes.string_view());
    const char* vs_cchar = vs_string.c_str();
    // vertex Shader
    vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vs_cchar, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        LOG_F(ERROR, "SHADER::VERTEX::COMPILATION_FAILED: {}", infoLog);
    };

    auto fs_bytes = nw::ByteArray::from_file(fragment);
    auto fs_string = std::string(fs_bytes.string_view());
    const char* fs_cchar = fs_string.c_str();
    // vertex Shader
    fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fs_cchar, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        LOG_F(ERROR, "SHADER::VERTEX::COMPILATION_FAILED: {}", infoLog);
    };

    // shader Program
    id_ = glCreateProgram();
    glAttachShader(id_, vs);
    glAttachShader(id_, fs);
    glLinkProgram(id_);
    // print linking errors if any
    glGetProgramiv(id_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        LOG_F(ERROR, "SHADER::VERTEX::LINKING_FAILED", infoLog);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void Shader::use()
{
    glUseProgram(id_);
}

void Shader::set_uniform(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::set_uniform(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::set_uniform(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(id_, name.c_str()), value);
}

void Shader::set_uniform(const std::string& name, const glm::mat4& value) const
{
    unsigned int transformLoc = glGetUniformLocation(id_, name.c_str());
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(value));
}
