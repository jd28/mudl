#include "model.hpp"

#include "TextureCache.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

static TextureCache textures;

bgfx::VertexLayout Node::layout;

void Node::submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4x4& _mtx, uint64_t _state)
{
    auto trans = glm::translate(_mtx, position_);

    for (auto child : children_) {
        child->submit(_id, _program, trans, _state);
    }
}

// == Mesh ===================================================================
// ============================================================================

void Mesh::submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state)
{
    static bgfx::UniformHandle s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

    if (BGFX_STATE_MASK == _state) {
        _state = 0
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_CULL_CCW
            | BGFX_STATE_MSAA;
    }

    auto trans = glm::translate(_mtx, position_);
    trans = glm::rotate(trans, rotation_[3], {rotation_[0], rotation_[1], rotation_[2]});
    trans = glm::scale(trans, scale_);

    // LOG_F(INFO, "Submitting node: {}", orig->name);

    bgfx::setTransform(&trans[0][0]);
    bgfx::setState(_state);
    bgfx::setVertexBuffer(0, vbh_);
    bgfx::setIndexBuffer(ibh_);
    bgfx::setTexture(0, s_texColor, texture0);
    bgfx::submit(
        _id, _program, 0, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);

    bgfx::discard();

    for (auto child : children_) {
        child->submit(_id, _program, trans, _state);
    }
}

// == Model Loading ===========================================================
// ============================================================================

static inline Node* load_node(nw::model::Node* node)
{
    Node* result = nullptr;
    if (node->type & nw::model::NodeFlags::mesh) {
        auto n = static_cast<nw::model::TrimeshNode*>(node);
        if (!n->indices.empty()) {
            Mesh* mesh = new Mesh;
            LOG_F(INFO, "name: {} index size: {}", n->name, n->indices.size() / 3);
            auto index_mem = bgfx::makeRef(n->indices.data(), uint32_t(n->indices.size() * sizeof(uint16_t)));
            mesh->ibh_ = bgfx::createIndexBuffer(index_mem);

            auto mem = bgfx::alloc(uint32_t(n->vertices.size() * Node::layout.getStride()));

            uint32_t i = 0;
            for (const auto& v : n->vertices) {
                bgfx::vertexPack(&v.position.x, false, bgfx::Attrib::Position, Node::layout, mem->data, i);
                bgfx::vertexPack(&v.tex_coords.x, false, bgfx::Attrib::TexCoord0, Node::layout, mem->data, i);
                ++i;
            }

            mesh->vbh_ = bgfx::createVertexBuffer(mem, Node::layout);

            auto [pk, pdata] = n->get_controller(nw::model::ControllerType::Position);
            if (pdata.size() != 3) {
                LOG_F(FATAL, "Wrong size position: {}", pdata.size());
            }
            mesh->position_ = glm::vec3{pdata[0], pdata[1], pdata[2]};

            auto [ok, odata] = n->get_controller(nw::model::ControllerType::Orientation);
            if (odata.size() != 4) {
                LOG_F(FATAL, "Wrong size orientation: {}", odata.size());
            }
            mesh->orig = n;
            mesh->rotation_ = glm::vec4{odata[0], odata[1], odata[2], odata[3]};

            // Force tga for now
            auto tex = textures.load(n->bitmap, nw::ResourceType::tga);
            if (tex) {
                mesh->texture0 = *tex;
            } else {
                LOG_F(FATAL, "Failed to bind texture");
            }
            result = mesh;
        } else {
            LOG_F(ERROR, "No vertex indicies");
        }
    } else {
        result = new Node;
    }

    for (auto child : node->children) {
        result->children_.push_back(load_node(child));
    }
    return result;
}

Node* load_model(nw::model::Model* mdl)
{
    auto root = mdl->find("rootdummy");
    if (!root) {
        LOG_F(INFO, "No root dummy");
        return nullptr;
    }
    return load_node(root);
}
