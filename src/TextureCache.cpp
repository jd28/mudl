#include "TextureCache.hpp"

#include <nw/kernel/Resources.hpp>

void TextureCache::load_placeholder()
{
    place_holder_image_ = std::make_unique<nw::Image>("assets/templategrid_albedo.png");

    auto size = place_holder_image_->channels() * place_holder_image_->height() * place_holder_image_->width();
    auto mem = bgfx::makeRef(place_holder_image_->data(), size);
    auto handle = bgfx::createTexture2D(uint16_t(place_holder_image_->width()),
        uint16_t(place_holder_image_->height()), false, 1,
        place_holder_image_->channels() == 4 ? bgfx::TextureFormat::RGBA8 : bgfx::TextureFormat::RGB8,
        BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, mem);

    place_holder_ = handle;
}

std::optional<bgfx::TextureHandle> TextureCache::load(std::string_view resref,
    nw::ResourceType::type type)
{
    absl::string_view needle{resref.data(), resref.size()};
    auto it = map_.find(needle);
    if (it == std::end(map_)) {
        // Create
        auto [bytes, ty] = nw::kernel::resman().demand_in_order(resref, {type});
        if (bytes.size() == 0) {
            LOG_F(ERROR, "Failed to find texture: {} of type: {}", resref, ty);
            return place_holder_;
        }
        auto img = std::make_unique<nw::Image>(std::move(bytes), ty == nw::ResourceType::dds);
        if (!img->valid()) {
            LOG_F(ERROR, "Failed to load image: {} of type: {}", resref, ty);
            return place_holder_;
        }

        auto size = img->channels() * img->height() * img->width();
        auto mem = bgfx::makeRef(img->data(), size);
        auto handle = bgfx::createTexture2D(uint16_t(img->width()), uint16_t(img->height()), false, 1,
            img->channels() == 4 ? bgfx::TextureFormat::RGBA8 : bgfx::TextureFormat::RGB8,
            BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, mem);

        map_.insert({std::string(resref), TexturePayload{std::move(img), handle, 1}});
        return handle;
    } else {
        ++it->second.refcount_;
        return it->second.handle_;
    }
    // Failure
    return {};
}
