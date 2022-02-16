/**
 * @file   Materials.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2022.02.16
 *
 * @brief  Declaration of application materials.
 */

#pragma once

#include "main.h"
#include "gfx/Material.h"
#include "materials/material_sample_host_interface.h"

namespace vkfw_app::gfx {

    struct MirrorMaterialInfo : public vkfw_core::gfx::MaterialInfo
    {
        static constexpr std::uint32_t MATERIAL_ID = static_cast<std::uint32_t>(materials::MaterialIdentifierApp::MirrorMaterialType);

        MirrorMaterialInfo() : MaterialInfo("MirrorMaterial", MATERIAL_ID) {}
        MirrorMaterialInfo(std::string_view name) : MaterialInfo(name, MATERIAL_ID) {}

        /** Holds the materials reflectivity. */
        glm::vec3 m_Kr = glm::vec3{1.0f};

        static std::size_t GetGPUSize();
        static void FillGPUInfo(const MirrorMaterialInfo& info, std::span<std::uint8_t>& gpuInfo, std::uint32_t firstTextureIndex);
        std::unique_ptr<MaterialInfo> copy() override;

        template<class Archive> void serialize(Archive& ar, [[maybe_unused]] const std::uint32_t version) // NOLINT
        {
            ar(cereal::base_class<MaterialInfo>(this), cereal::make_nvp("Kr", m_Kr));
        }

    protected:
        MirrorMaterialInfo(std::string_view name, std::uint32_t materialId) : MaterialInfo(name, materialId) {}
    };

}
