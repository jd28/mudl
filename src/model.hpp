#pragma once

#include <nw/model/Mdl.hpp>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/matrix.hpp>

#include <vector>

struct Transform {
};

struct Node {
    static bgfx::VertexLayout layout;

    virtual ~Node() = default;
    virtual void reset() { }

    // Submits mesh data to the GPU
    virtual void submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4x4& _mtx, uint64_t _state = BGFX_STATE_MASK);

    bool has_transform_ = false;
    glm::vec3 position_{0.0f};
    glm::vec4 rotation_{0.0f};
    glm::vec3 scale_ = glm::vec3(1.0);
    std::vector<Node*> children_;
};

struct Mesh : public Node {
    virtual void reset() override { }
    // Submits mesh data to the GPU
    virtual void submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state = BGFX_STATE_MASK) override;

    nw::model::TrimeshNode* orig = nullptr;
    bgfx::VertexBufferHandle vbh_;
    bgfx::IndexBufferHandle ibh_;
    uint16_t num_vertices_ = 0;
    uint8_t* vertices_ = nullptr;
    uint32_t num_indices_ = 0;
    uint16_t* indices_ = nullptr;

    bgfx::TextureHandle texture0;
    // bgfx::TextureHandle texture1;
    // bgfx::TextureHandle texture2;
    // bgfx::TextureHandle texture3;

    // bx::Sphere sphere_;
    // bx::Aabb   aabb_;
    // bx::Obb    obb_;
    // PrimitiveArray prims_;
};

Node* load_model(nw::model::Model* mdl);
