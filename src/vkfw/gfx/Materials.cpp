/**
 * @file   Materials.cpp
 * @author Sebastian Maisch <sebastian.maisch@googlemail.com>
 * @date   2022.02.16
 *
 * @brief  Implementation of the application materials.
 */

#include "gfx/Materials.h"

namespace vkfw_app::gfx {
    std::size_t MirrorMaterialInfo::GetGPUSize() { return sizeof(materials::MirrorMaterial); }

    void MirrorMaterialInfo::FillGPUInfo(const MirrorMaterialInfo& info, std::span<std::uint8_t>& gpuInfo, [[maybe_unused]] std::uint32_t firstTextureIndex)
    {
        auto mat = reinterpret_cast<materials::MirrorMaterial*>(gpuInfo.data());
        mat->Kr = info.m_Kr;
    }

    std::unique_ptr<vkfw_core::gfx::MaterialInfo> MirrorMaterialInfo::copy() { return std::make_unique<MirrorMaterialInfo>(*this); }
}
