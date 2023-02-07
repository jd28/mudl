#pragma once

#include <absl/container/flat_hash_map.h>
#include <nw/legacy/Image.hpp>
#include <nw/legacy/Plt.hpp>
#include <nw/resources/ResourceType.hpp>

#include <array>
#include <optional>
#include <variant>

struct TexturePayload {
    std::variant<std::unique_ptr<nw::Image>, std::unique_ptr<nw::Plt>> image_;
    unsigned int handle_;
    bool is_plt = false;
    uint32_t refcount_ = 0;
};

struct TextureCache {
    void load_placeholder();
    void load_palette_texture();

    std::optional<std::pair<unsigned int, bool>> load(std::string_view resref);
    absl::flat_hash_map<std::string, TexturePayload> map_;

    unsigned int place_holder_;
    std::unique_ptr<nw::Image> place_holder_image_;
    std::array<unsigned int, 10> palette_texture_;
};
