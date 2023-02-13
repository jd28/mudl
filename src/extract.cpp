#include "extract.hpp"

#include <nw/kernel/Resources.hpp>
#include <nw/model/Mdl.hpp>

#include <regex>

void extract(std::string_view resref)
{
    nw::Resource res{resref, nw::ResourceType::mdl};
    nw::model::Mdl mdl{nw::kernel::resman().demand(res)};
    nw::kernel::resman().extract_by_glob(res.filename(), ".");
    for (const auto& node : mdl.model.nodes) {
        if (node->type & nw::model::NodeFlags::mesh) {
            auto n = static_cast<const nw::model::TrimeshNode*>(node.get());
            if (n->bitmap.size()) {
                auto re = std::regex{n->bitmap + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                nw::kernel::resman().extract(re, ".");
            }

            if (n->materialname.size()) {
                auto re = std::regex{n->materialname + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                nw::kernel::resman().extract(re, ".");
            }

            if (n->textures[0].size()) {
                auto re = std::regex{n->textures[0] + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                nw::kernel::resman().extract(re, ".");
            }

            if (n->textures[1].size()) {
                auto re = std::regex{n->textures[1] + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                nw::kernel::resman().extract(re, ".");
            }

            if (n->textures[2].size()) {
                auto re = std::regex{n->textures[2] + "\\.(mtr|dds|plt|tga|txi)", std::regex_constants::icase};
                nw::kernel::resman().extract(re, ".");
            }
        }
    }
}
