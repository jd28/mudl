#pragma once

#include "model.hpp"

#include <absl/container/flat_hash_map.h>

#include <memory>

struct ModelPayload {
    std::unique_ptr<Model> model_;
    std::unique_ptr<nw::model::Mdl> original_;
    uint32_t refcount_ = 0;
};

struct ModelCache {
    Model* load(std::string_view resref);
    absl::flat_hash_map<std::string, ModelPayload> map_;
};
