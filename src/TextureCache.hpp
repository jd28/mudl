#pragma once

#include <absl/container/flat_hash_map.h>
#include <bgfx/bgfx.h>
#include <nw/legacy/Image.hpp>
#include <nw/resources/ResourceType.hpp>

#include <optional>

struct TexturePayload {
    std::unique_ptr<nw::Image> image_;
    bgfx::TextureHandle handle_;
    uint32_t refcount_ = 0;
};

struct TextureCache {
    void load_placeholder();

    std::optional<bgfx::TextureHandle> load(std::string_view resref);
    absl::flat_hash_map<std::string, TexturePayload> map_;

    bgfx::TextureHandle place_holder_;
    std::unique_ptr<nw::Image> place_holder_image_;
};
