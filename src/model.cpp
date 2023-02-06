#include "model.hpp"

#include "TextureCache.hpp"
#include "shader.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <glad/glad.h>

extern TextureCache s_textures;

#define CHECK_GL_ERRORS()                          \
    do {                                           \
        auto err = glGetError();                   \
        if (err) {                                 \
            LOG_F(ERROR, "openGL error: {}", err); \
        }                                          \
    } while (0)

void Node::draw(Shader& shader, const glm::mat4x4& mtx)
{
    auto trans = glm::translate(mtx, position_);

    for (auto child : children_) {
        child->draw(shader, trans);
    }
}

// // == Mesh ===================================================================
// // ============================================================================

void Mesh::draw(Shader& shader, const glm::mat4x4& mtx)
{
    auto trans = glm::translate(mtx, position_);
    // trans = glm::rotate(trans, rotation_[3], {rotation_[0], rotation_[1], rotation_[2]});
    trans = glm::scale(trans, scale_);

    shader.set_uniform("model", trans);

    auto diffuse_loc = glGetUniformLocation(shader.id_, "ourTexture");
    auto plt_tex_loc = glGetUniformLocation(shader.id_, "paletteTexture");

    // Then bind the uniform samplers to texture units:
    shader.use();
    glUniform1i(diffuse_loc, 0);
    glUniform1i(plt_tex_loc, 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture0);

    if (texture0_is_plt) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D_ARRAY, s_textures.palette_texture_);
        CHECK_GL_ERRORS();
    }

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, orig->indices.size(), GL_UNSIGNED_SHORT, 0);
    CHECK_GL_ERRORS();
    glBindVertexArray(0);

    for (auto child : children_) {
        child->draw(shader, trans);
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

            glGenVertexArrays(1, &mesh->vao_);
            glGenBuffers(1, &mesh->vbo_);
            glGenBuffers(1, &mesh->ebo_);

            glBindVertexArray(mesh->vao_);
            glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo_);

            glBufferData(GL_ARRAY_BUFFER, n->vertices.size() * sizeof(nw::model::Vertex), &n->vertices[0],
                GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, n->indices.size() * sizeof(uint16_t),
                &n->indices[0], GL_STATIC_DRAW);

            // vertex positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(nw::model::Vertex), (void*)0);

            // vertex texture coords
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(nw::model::Vertex),
                (void*)offsetof(nw::model::Vertex, tex_coords));

            // vertex normals
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(nw::model::Vertex),
                (void*)offsetof(nw::model::Vertex, normal));

            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(nw::model::Vertex),
                (void*)offsetof(nw::model::Vertex, tangent));

            glBindVertexArray(0);

            auto key = n->get_controller(nw::model::ControllerType::Position);
            if (key.data.size() != 3) {
                LOG_F(FATAL, "Wrong size position: {}", key.data.size());
            }
            mesh->position_ = glm::vec3{key.data[0], key.data[1], key.data[2]};

            key = n->get_controller(nw::model::ControllerType::Orientation);
            if (key.data.size() != 4) {
                LOG_F(FATAL, "Wrong size orientation: {}", key.data.size());
            }
            mesh->orig = n;
            mesh->rotation_ = glm::vec4{key.data[0], key.data[1], key.data[2], key.data[3]};

            auto tex = s_textures.load(n->bitmap);
            if (tex) {
                mesh->texture0 = tex->first;
                mesh->texture0_is_plt = tex->second;
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
    auto root = mdl->find(std::regex(mdl->name));
    LOG_F(INFO, "{}", root->children.size());
    if (!root) {
        LOG_F(INFO, "No root dummy");
        return nullptr;
    }
    return load_node(root);
}
