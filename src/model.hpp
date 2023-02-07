#pragma once

#include <nw/legacy/Plt.hpp>
#include <nw/model/Mdl.hpp>

#include <glm/glm.hpp>
#include <glm/matrix.hpp>

#include <vector>

struct Shader;

struct Transform {
};

struct Node {
    // static bgfx::VertexLayout layout;

    virtual ~Node() = default;
    virtual void reset() { }

    // Submits mesh data to the GPU
    // virtual void submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4x4& _mtx, uint64_t _state = BGFX_STATE_MASK);

    virtual void draw(Shader& shader, const glm::mat4x4& mtx);

    bool is_root_ = true;
    glm::vec3 position_{0.0f};
    glm::vec4 rotation_{0.0f};
    glm::vec3 scale_ = glm::vec3(1.0);
    std::vector<Node*> children_;
};

struct Mesh : public Node {
    virtual void reset() override { }
    // Submits mesh data to the GPU
    // virtual void submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state = BGFX_STATE_MASK) override;

    virtual void draw(Shader& shader, const glm::mat4x4& mtx) override;

    nw::model::TrimeshNode* orig = nullptr;
    unsigned int vao_, vbo_, ebo_;
    uint16_t num_vertices_ = 0;
    uint8_t* vertices_ = nullptr;
    uint32_t num_indices_ = 0;
    uint16_t* indices_ = nullptr;
    nw::PltColors plt_colors_{};

    unsigned int texture0;
    // bgfx::TextureHandle texture1;
    // bgfx::TextureHandle texture2;
    // bgfx::TextureHandle texture3;

    // bx::Sphere sphere_;
    // bx::Aabb   aabb_;
    // bx::Obb    obb_;
    // PrimitiveArray prims_;

    bool texture0_is_plt = false;
};

Node* load_model(nw::model::Model* mdl);
