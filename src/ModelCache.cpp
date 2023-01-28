#include "ModelCache.hpp"

#include <nw/kernel/Resources.hpp>

Node* ModelCache::load(std::string_view resref)
{
    absl::string_view needle{resref.data(), resref.size()};
    auto it = map_.find(needle);
    if (it == std::end(map_)) {
        auto bytes = nw::kernel::resman().demand({resref, nw::ResourceType::mdl});
        if (bytes.size() == 0) {
            LOG_F(ERROR, "Failed to find model: {}", resref);
            return nullptr;
        }

        auto model = std::make_unique<nw::model::Mdl>(std::move(bytes));
        if (!model->valid()) {
            LOG_F(ERROR, "Failed to parse model: {}", resref);
            return nullptr;
        }
        auto node = load_model(&model.get()->model);
        if (!node) {
            LOG_F(ERROR, "Failed to load model: {}", resref);
            return nullptr;
        }
        map_.emplace(std::string(resref), ModelPayload{std::unique_ptr<Node>(node), std::move(model), 1});
        return node;
    } else {
        ++it->second.refcount_;
        return it->second.model_.get();
    }

    // Failure
    return nullptr;
}
