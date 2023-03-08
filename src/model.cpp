#include "model.hpp"

#include "TextureCache.hpp"

#include <glm/gtc/quaternion.hpp>

extern TextureCache s_textures;
bgfx::VertexLayout Node::layout;

// == Node ====================================================================
// ============================================================================

Node* Node::find(std::string_view name)
{
    if (nw::string::icmp(orig_->name, name)) {
        return this;
    }

    for (const auto child : children_) {
        auto n = child->find(name);
        if (n) { return n; }
    }

    return nullptr;
}

void Node::submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4x4& _mtx, uint64_t _state)
{
    glm::mat4x4 trans;

    if (has_transform_) {
        trans = glm::translate(_mtx, position_);
        trans = trans * glm::toMat4(rotation_);
    } else {
        trans = _mtx;
    }

    for (auto child : children_) {
        child->submit(_id, _program, trans, _state);
    }
}

// == Model ===================================================================
// ============================================================================

bool Model::load_animation(std::string_view anim)
{
    anim_ = nullptr;
    nw::model::Model* m = mdl_;
    while (m) {
        for (const auto& it : m->animations) {
            if (it->name == anim) {
                anim_ = it.get();
                break;
            }
        }
        if (!m->supermodel || anim_) { break; }
        m = &m->supermodel->model;
    }
    if (anim_) {
        LOG_F(INFO, "Loaded animation: {} from model: {}", anim, m->name);
    }
    return !!anim_;
}

void Model::update(int32_t dt)
{
    if (!anim_) { return; }

    if (dt + anim_cursor > int32_t(anim_->length * 1000)) {
        anim_cursor = dt + anim_cursor - int32_t(anim_->length * 1000);
    } else {
        anim_cursor += dt;
    }

    for (const auto& anim : anim_->nodes) {
        // LOG_F(INFO, "animation node: {}, cursor: {}", anim->name, anim_cursor);
        auto node = find(anim->name);
        if (node) {
            auto poskey = anim->get_controller(nw::model::ControllerType::Position, true);
            // LOG_F(INFO, "time size: {}, data size: {}", poskey.time.size(), poskey.data.size());
            if (poskey.time.size()) {
                int i = 0;
                int start = 0, end = 0;
                while (i < poskey.time.size() && anim_cursor >= int32_t(poskey.time[i] * 1000)) {
                    ++i;
                }
                end = i;
                if (end >= poskey.time.size()) { end = 0; }
                // start = i - 1;
                // if (start < 0) { start = poskey.time.size() - 1; }
                node->position_ = glm::vec3{poskey.data[end * 3], poskey.data[end * 3 + 1], poskey.data[end * 3 + 2]};
            }

            auto orikey = anim->get_controller(nw::model::ControllerType::Orientation, true);
            if (orikey.time.size()) {
                int i = 0;
                int start = 0, end = 0;
                while (i < orikey.time.size() && anim_cursor >= int32_t(orikey.time[i] * 1000)) {
                    ++i;
                }
                end = i;
                if (end >= orikey.time.size()) { end = 0; }
                // start = i - 1;
                // if (start < 0) { start = poskey.time.size() - 1; }
                node->rotation_ = glm::qua{
                    orikey.data[end * 4 + 3],
                    orikey.data[end * 4],
                    orikey.data[end * 4 + 1],
                    orikey.data[end * 4 + 2],
                };
                // LOG_F(INFO, "anim: {}, key: {}, {}, {}, {}", anim->name,
                //     orikey.data[end * 4],
                //     orikey.data[end * 4 + 1],
                //     orikey.data[end * 4 + 2],
                //     orikey.data[end * 4 + 3]);
            }
        }
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
    trans = trans * glm::toMat4(rotation_);
    trans = glm::scale(trans, scale_);

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

static inline Node* load_node(nw::model::Node* node, bool is_root)
{
    Node* result = nullptr;
    if (is_root) {
        result = new Model;
    } else if (node->type & nw::model::NodeFlags::mesh && !(node->type & nw::model::NodeFlags::aabb)) {
        auto n = static_cast<nw::model::TrimeshNode*>(node);
        if (!n->indices.empty()) {
            Mesh* mesh = new Mesh;
            LOG_F(INFO, "name: {} index size: {}", n->name, n->indices.size() / 3);
            auto index_mem = bgfx::makeRef(n->indices.data(), uint32_t(n->indices.size() * sizeof(uint16_t)));
            mesh->ibh_ = bgfx::createIndexBuffer(index_mem);

            auto mem = bgfx::makeRef(n->vertices.data(), uint32_t(n->vertices.size() * Node::layout.getStride()));
            mesh->vbh_ = bgfx::createVertexBuffer(mem, Node::layout);

            // Force tga for now
            auto tex = s_textures.load(n->bitmap);
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
    result->orig_ = node;

    auto key = node->get_controller(nw::model::ControllerType::Position);
    if (key.data.size()) {
        result->has_transform_ = true;
        if (key.data.size() != 3) {
            LOG_F(FATAL, "Wrong size position: {}", key.data.size());
        }
        result->position_ = glm::vec3{key.data[0], key.data[1], key.data[2]};

        key = node->get_controller(nw::model::ControllerType::Orientation);
        if (key.data.size() != 4) {
            LOG_F(FATAL, "Wrong size orientation: {}", key.data.size());
        }
        result->rotation_ = glm::qua{key.data[3], key.data[0], key.data[1], key.data[2]};
    }

    for (auto child : node->children) {
        result->children_.push_back(load_node(child, false));
    }
    return result;
}

Model* load_model(nw::model::Model* mdl)
{
    auto root = mdl->find(std::regex(mdl->name));
    if (!root) {
        LOG_F(INFO, "No root dummy");
        return nullptr;
    }
    auto result = static_cast<Model*>(load_node(root, true));
    result->mdl_ = mdl;
    return result;
}
