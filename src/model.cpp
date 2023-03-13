#include "model.hpp"

#include "TextureCache.hpp"
#include "util.hpp"

#include <glm/gtc/quaternion.hpp>

extern TextureCache s_textures;
bgfx::VertexLayout Node::layout;
bgfx::ProgramHandle Node::skinned_program;

// == Node ====================================================================
// ============================================================================

glm::mat4 Node::get_transform() const
{
    auto parent = glm::mat4{1.0f};
    if (!has_transform_) { return parent; }
    if (parent_) {
        parent = parent_->get_transform();
    }

    auto trans = glm::translate(parent, position_);
    trans = trans * glm::toMat4(rotation_);
    trans = glm::scale(trans, scale_);

    return trans;
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

Node* Model::find(std::string_view name)
{
    for (const auto& node : nodes_) {
        if (nw::string::icmp(node->orig_->name, name)) {
            return node.get();
        }
    }

    return nullptr;
}

void Model::initialize_skins()
{
    for (auto& node : nodes_) {
        if (auto skin = dynamic_cast<Skin*>(node.get())) {
            auto orig = static_cast<nw::model::SkinNode*>(skin->orig_);
            for (size_t i = 0; i < 64; ++i) {
                if (orig->bone_nodes[i] == -1 || orig->bone_nodes[i] >= nodes_.size()) {
                    break;
                }
                auto bone_node = nodes_[orig->bone_nodes[i]].get();
                bone_node->no_render_ = true;
                skin->inverse_binds_[i] = glm::inverse(bone_node->get_transform());
            }
        }
    }
}

bool Model::load(nw::model::Model* mdl)
{
    auto root = mdl->find(std::regex(mdl->name));
    if (!root) {
        LOG_F(INFO, "No root dummy");
        return false;
    }
    if (load_node(root)) {
        mdl_ = mdl;
        for (auto& node : nodes_) {
            node->owner_ = this;
        }
        initialize_skins();
        return true;
    }
    return false;
}

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

Node* Model::load_node(nw::model::Node* node, Node* parent)
{
    Node* result = nullptr;
    if (node->type & nw::model::NodeFlags::skin) {
        auto n = static_cast<nw::model::SkinNode*>(node);
        if (!n->indices.empty()) {
            Skin* skin = new Skin;
            LOG_F(INFO, "name: {} index size: {}", n->name, n->indices.size() / 3);
            auto index_mem = bgfx::makeRef(n->indices.data(), uint32_t(n->indices.size() * sizeof(uint16_t)));
            skin->ibh_ = bgfx::createIndexBuffer(index_mem);

            auto mem = bgfx::makeRef(n->vertices.data(), uint32_t(n->vertices.size() * Skin::layout.getStride()));
            skin->vbh_ = bgfx::createVertexBuffer(mem, Skin::layout);

            auto tex = s_textures.load(n->bitmap);
            if (tex) {
                skin->texture0 = *tex;
            } else {
                LOG_F(FATAL, "Failed to bind texture");
            }

            result = skin;
        } else {
            LOG_F(ERROR, "No vertex indicies");
        }
    } else if (node->type & nw::model::NodeFlags::mesh && !(node->type & nw::model::NodeFlags::aabb)) {
        auto n = static_cast<nw::model::TrimeshNode*>(node);
        if (!n->indices.empty()) {
            Mesh* mesh = new Mesh;
            LOG_F(INFO, "name: {} index size: {}", n->name, n->indices.size() / 3);
            auto index_mem = bgfx::makeRef(n->indices.data(), uint32_t(n->indices.size() * sizeof(uint16_t)));
            mesh->ibh_ = bgfx::createIndexBuffer(index_mem);

            auto mem = bgfx::makeRef(n->vertices.data(), uint32_t(n->vertices.size() * Node::layout.getStride()));
            mesh->vbh_ = bgfx::createVertexBuffer(mem, Node::layout);

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
    result->parent_ = parent;
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

    nodes_.emplace_back(result);
    for (auto child : node->children) {
        result->children_.push_back(load_node(child, result));
    }

    return result;
}

void Model::update(int32_t dt)
{
    if (!anim_) { return; }

    if (dt + anim_cursor_ > int32_t(anim_->length * 1000)) {
        anim_cursor_ = dt + anim_cursor_ - int32_t(anim_->length * 1000);
    } else {
        anim_cursor_ += dt;
    }

    for (const auto& anim : anim_->nodes) {
        // LOG_F(INFO, "animation node: {}, cursor: {}", anim->name, anim_cursor_);
        auto node = find(anim->name);
        if (node) {
            auto poskey = anim->get_controller(nw::model::ControllerType::Position, true);
            // LOG_F(INFO, "time size: {}, data size: {}", poskey.time.size(), poskey.data.size());
            if (poskey.time.size()) {
                int i = 0;
                int start = 0, end = 0;
                while (i < poskey.time.size() && anim_cursor_ >= int32_t(poskey.time[i] * 1000)) {
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
                while (i < orikey.time.size() && anim_cursor_ >= int32_t(orikey.time[i] * 1000)) {
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
            }
        }
    }
}

void Model::submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state)
{
    nodes_[0]->submit(_id, _program, _mtx, _state);
}

// == Mesh ===================================================================
// ============================================================================

void Mesh::submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state)
{
    static bgfx::UniformHandle s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

    auto trans = glm::translate(_mtx, position_);
    trans = trans * glm::toMat4(rotation_);
    trans = glm::scale(trans, scale_);

    if (!no_render_) {
        if (BGFX_STATE_MASK == _state) {
            _state = 0
                | BGFX_STATE_WRITE_RGB
                | BGFX_STATE_WRITE_A
                | BGFX_STATE_WRITE_Z
                | BGFX_STATE_DEPTH_TEST_LESS
                | BGFX_STATE_CULL_CCW
                | BGFX_STATE_MSAA;
        }

        bgfx::setTransform(&trans[0][0]);
        bgfx::setState(_state);
        bgfx::setVertexBuffer(0, vbh_);
        bgfx::setIndexBuffer(ibh_);
        bgfx::setTexture(0, s_texColor, texture0);
        bgfx::submit(
            _id, _program, 0, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);

        bgfx::discard();
    }
    for (auto child : children_) {
        child->submit(_id, _program, trans, _state);
    }
}

// == Skin ====================================================================
// ============================================================================

bgfx::VertexLayout Skin::layout;

void Skin::submit(bgfx::ViewId _id, bgfx::ProgramHandle _program, const glm::mat4& _mtx, uint64_t _state)
{
    static bgfx::UniformHandle s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
    static bgfx::UniformHandle u_joints = bgfx::createUniform("u_joints", bgfx::UniformType::Mat4, 64);

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

    auto orig = static_cast<nw::model::SkinNode*>(orig_);
    for (size_t i = 0; i < 64; ++i) {
        if (orig->bone_nodes[i] == -1 || size_t(orig->bone_nodes[i]) >= owner_->nodes_.size()) {
            break;
        }
        auto bone_node = owner_->nodes_[orig->bone_nodes[i]].get();
        joints_[i] = bone_node->get_transform() * inverse_binds_[i];
    }

    bgfx::setTransform(&trans[0][0]);
    bgfx::setState(_state);
    bgfx::setVertexBuffer(0, vbh_);
    bgfx::setIndexBuffer(ibh_);
    bgfx::setTexture(0, s_texColor, texture0);
    bgfx::setUniform(u_joints, joints_.data(), UINT16_MAX);
    bgfx::submit(
        _id, Node::skinned_program, 0, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);

    bgfx::discard();

    for (auto child : children_) {
        child->submit(_id, _program, trans, _state);
    }
}
