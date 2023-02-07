#include "model.hpp"

#include "ModelCache.hpp"
#include "TextureCache.hpp"
#include "shader.hpp"
#include "util.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <nw/kernel/Kernel.hpp>
#include <nw/kernel/Resources.hpp>
#include <nw/legacy/Image.hpp>
#include <nw/model/Mdl.hpp>
#include <nw/util/string.hpp>
#include <stb/stb_image.h>

#include "imgui.h"

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <iostream>
#include <limits>
#include <regex>
#include <string>
#include <vector>

using namespace std::literals;

static ModelCache s_models;
TextureCache s_textures;

auto usage = R"eof(usage: mudl <command> [<args>]

Commands
--------
    extract     Extracts a model and all its corresponding textures
    info        Prints data regarding the structure of the model
    view        Open a model for viewing
)eof";

auto extract_usage = R"eof(usage: mudl extract <resref>
)eof";

auto view_usage = R"eof(usage: mudl view <resref>
)eof";

void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

double prev_mouse_x = 0;
double prev_mouse_y = 0;
float cam_pitch = 0.0f;
float cam_yaw = 0.0f;
float rot_scale = 0.01f;

glm::vec3 cameraPos = glm::vec3(0.0f, -0.5f, 2.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    const float cameraSpeed = 0.05f; // adjust accordingly
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

bool firstMouse = true;
float lastX = 400, lastY = 300;
float yaw, pitch;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cout << usage;
        return 1;
    }

    nw::init_logger(argc, argv);
    auto info = nw::probe_nwn_install();
    nw::kernel::config().initialize({
        info.version,
        info.install,
        info.user,
    });
    nw::kernel::resman().add_container(new nw::Directory("assets"));
    nw::kernel::services().start();

    if ("extract"sv == argv[1]) {
        if (argc < 3) {
            std::cout << extract_usage;
            return 1;
        }
        // Proof of concept, one hardcoded model, obviously this is stupid.
        nw::Resource res{std::string_view(argv[2]), nw::ResourceType::mdl};
        nw::model::Mdl mdl{nw::kernel::resman().demand(res)};
        nw::kernel::resman().extract_by_glob(res.filename(), ".");
        for (const auto& node : mdl.model.nodes) {
            if (node->type & nw::model::NodeFlags::mesh) {
                auto n = static_cast<const nw::model::TrimeshNode*>(node.get());
                if (n->bitmap.size()) {
                    auto re = std::regex{n->bitmap + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                    nw::kernel::resman().extract(re, ".");
                }

                if (n->materialname.size()) {
                    auto re = std::regex{n->materialname + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                    nw::kernel::resman().extract(re, ".");
                }

                if (n->textures[0].size()) {
                    auto re = std::regex{n->textures[0] + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                    nw::kernel::resman().extract(re, ".");
                }

                if (n->textures[1].size()) {
                    auto re = std::regex{n->textures[1] + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                    nw::kernel::resman().extract(re, ".");
                }

                if (n->textures[2].size()) {
                    auto re = std::regex{n->textures[2] + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                    nw::kernel::resman().extract(re, ".");
                }
            }
        }

    } else if ("view"sv == argv[1]) {
        if (argc < 3) {
            std::cout << view_usage;
            return 1;
        }

        const int width = 800;
        const int height = 600;

        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        GLFWwindow* window = glfwCreateWindow(width, height, "tst", NULL, NULL);
        if (window == NULL) {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return -1;
        }

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cout << "Failed to initialize GLAD" << std::endl;
            return -1;
        }
        glEnable(GL_DEPTH_TEST);

        s_textures.load_placeholder();
        s_textures.load_palette_texture();

        auto node = s_models.load(argv[2]);
        if (!node) {
            LOG_F(ERROR, "failed to load model {}", argv[2]);
            glfwTerminate();
            return 1;
        }

        Shader shader{"assets/vs_mudl.vert", "assets/fs_plt_mudl.frag"};

        while (!glfwWindowShouldClose(window)) {
            processInput(window);

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            shader.use();
            auto view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
            auto proj = glm::perspective(glm::radians(60.f), float(width) / float(height), 0.1f, 100.0f);
            shader.set_uniform("view", view);
            shader.set_uniform("projection", proj);

            glm::mat4 mtx = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), {0.0f, 0.0f, 1.0f});
            mtx = glm::rotate(mtx, glm::radians(90.0f), {1.0f, 0.0f, 0.0f});
            //  mtx = glm::rotate(mtx, glm::radians(90.0f), {.0f, 0.0f, 0.0f});
            node->draw(shader, mtx);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        glfwTerminate();
    } else {
        std::cout << usage;
        return 1;
    }
    return 0;
}
