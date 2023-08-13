#include "ModelCache.hpp"

#include <nw/kernel/Resources.hpp>

Model* ModelCache::load(std::string_view resref)
{
    auto it = map_.find(std::string(resref));
    if (it == std::end(map_)) {
        auto rd = nw::kernel::resman().demand({resref, nw::ResourceType::mdl});
        if (rd.bytes.size() == 0) {
            LOG_F(ERROR, "Failed to find model: {}", resref);
            return nullptr;
        }

        auto model = std::make_unique<nw::model::Mdl>(std::move(rd));
        if (!model->valid()) {
            LOG_F(ERROR, "Failed to parse model: {}", resref);
            return nullptr;
        }
        auto mdl = new Model;
        if (!mdl->load(&model.get()->model)) {
            LOG_F(ERROR, "Failed to load model: {}", resref);
            return nullptr;
        }
        std::string name = std::string(rd.name.resref.view());
        LOG_F(INFO, "Resref: {}", name);
        map_.insert({name, ModelPayload{std::unique_ptr<Model>(mdl), std::move(model), 1}});
        return mdl;
    } else {
        ++it->second.refcount_;
        return it->second.model_.get();
    }

    // Failure
    return nullptr;
}
