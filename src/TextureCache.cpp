#include "TextureCache.hpp"

#include <nw/kernel/Resources.hpp>

#include <glad/glad.h>

std::pair<unsigned int, bool> load_texture(std::string_view resref)
{

    unsigned int texture = std::numeric_limits<unsigned int>::max();

    auto [bytes, restype] = nw::kernel::resman().demand_in_order(resref,
        {nw::ResourceType::dds, nw::ResourceType::plt, nw::ResourceType::tga});

    if (bytes.size() == 0) {
        LOG_F(ERROR, "Failed to locate image: {}", resref);
        return {texture, false};
    }

    if (restype != nw::ResourceType::plt) {
        nw::Image img{std::move(bytes), restype == nw::ResourceType::dds};
        if (!img.valid()) {
            LOG_F(ERROR, "Failed to load image: {}.{}", resref, nw::ResourceType::to_string(restype));
            return {texture, false};
        }
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        auto format = img.channels() == 4 ? GL_RGBA : GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, img.width(), img.height(), 0, format, GL_UNSIGNED_BYTE,
            img.data());
        glGenerateMipmap(GL_TEXTURE_2D);
        return {texture, false};
    } else {
        nw::Plt plt{std::move(bytes)};
        if (!plt.valid()) {
            LOG_F(ERROR, "Failed to load image: {}.{}", resref, nw::ResourceType::to_string(restype));
            return {texture, false};
        }
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        // set the texture wrapping/filtering options (on the currently bound texture object)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        auto format = GL_RG;

        glTexImage2D(GL_TEXTURE_2D, 0, format, plt.width(), plt.height(), 0, format, GL_UNSIGNED_BYTE,
            plt.pixels());
        glGenerateMipmap(GL_TEXTURE_2D);
        return {texture, true};
    }

    return {texture, false};
}

void TextureCache::load_placeholder()
{
    place_holder_image_ = std::make_unique<nw::Image>("assets/templategrid_albedo.png");

    unsigned int texture = std::numeric_limits<unsigned int>::max();
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    auto format = place_holder_image_->channels() == 4 ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format,
        place_holder_image_->width(),
        place_holder_image_->height(), 0, format, GL_UNSIGNED_BYTE,
        place_holder_image_->data());
    glGenerateMipmap(GL_TEXTURE_2D);

    place_holder_ = texture;
}

void TextureCache::load_palette_texture()
{
    unsigned int texture = std::numeric_limits<unsigned int>::max();
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 256, 256, 10);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP);

    nw::ByteArray bytes;
    for (uint8_t i = 0; i < 10; ++i) {
        auto img = nw::kernel::resman().palette_texture(static_cast<nw::PltLayer>(i));
        bytes.append(img->data(), 4 * img->height() * img->width());
    }
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 256, 256, 10, GL_RGBA, GL_UNSIGNED_BYTE, bytes.data());
    palette_texture_ = texture;
}

std::optional<std::pair<unsigned int, bool>> TextureCache::load(std::string_view resref)
{
    absl::string_view needle{resref.data(), resref.size()};
    auto it = map_.find(needle);
    if (it == std::end(map_)) {
        // Create
        auto [texture, is_plt] = load_texture(resref);
        if (texture == std::numeric_limits<unsigned int>::max()) {
            return std::make_pair(place_holder_, false);
        }
        auto i = map_.emplace(resref, TexturePayload{{}, texture, is_plt, 1});
        return std::make_pair(i.first->second.handle_, i.first->second.is_plt);
    } else {
        ++it->second.refcount_;
        return std::make_pair(it->second.handle_, it->second.is_plt);
    }
    // Failure
    return std::make_pair(place_holder_, false);
}
