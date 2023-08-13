#pragma once

#include <nw/model/Mdl.hpp>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/matrix.hpp>

#include <vector>

struct Model;

struct Node {
    static bgfx::VertexLayout layout;
    static bgfx::ProgramHandle skinned_program;

    virtual ~Node() = default;
    virtual void reset() { }

    glm::mat4 get_transform() const;

    /// Submits mesh data to the GPU
    virtual void submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4x4& _mtx, uint64_t _state = BGFX_STATE_MASK);

    Model* owner_ = nullptr;
    nw::model::Node* orig_ = nullptr;
    Node* parent_ = nullptr;
    bool has_transform_ = false;
    glm::vec3 position_{0.0f};
    glm::quat rotation_{};
    glm::vec3 scale_ = glm::vec3(1.0);
    std::vector<Node*> children_;
    bool no_render_ = false;
    glm::mat4 inverse_{1.0f};
};

struct Model : public Node {
    nw::model::Model* mdl_ = nullptr;
    nw::model::Animation* anim_ = nullptr;
    int32_t anim_cursor_ = 0;
    std::vector<std::unique_ptr<Node>> nodes_;

    /// Finds a node by name
    Node* find(std::string_view name);

    /// Initialize skin meshes & joints
    void initialize_skins();

    /// Loads model from a NWN model
    bool load(nw::model::Model* mdl);

    /// Loads an animation
    bool load_animation(std::string_view anim);
    Node* load_node(nw::model::Node* node, Node* parent = nullptr);
    void update(int32_t dt);
    virtual void submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state = BGFX_STATE_MASK) override;
};

struct Mesh : public Node {
    virtual void reset() override { }
    // Submits mesh data to the GPU
    virtual void submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state = BGFX_STATE_MASK) override;

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

struct Skin : public Node {
    static bgfx::VertexLayout layout;

    virtual void reset() override { }
    // Submits mesh data to the GPU
    virtual void submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state = BGFX_STATE_MASK) override;

    void build_inverse_binds();
    bgfx::VertexBufferHandle vbh_;
    bgfx::IndexBufferHandle ibh_;
    uint16_t num_vertices_ = 0;
    uint8_t* vertices_ = nullptr;
    uint32_t num_indices_ = 0;
    uint16_t* indices_ = nullptr;
    std::vector<glm::mat4> inverse_bind_pose_;
    std::array<glm::mat4, 64> joints_;

    bgfx::TextureHandle texture0;
};

Model* load_model(nw::model::Model* mdl);
