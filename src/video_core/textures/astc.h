// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstdint>
#include <vector>

namespace Tegra::Texture::ASTC {

std::vector<uint8_t> Decompress(const uint8_t* data, uint32_t width, uint32_t height,
                                uint32_t depth, uint32_t block_width, uint32_t block_height);

} // namespace Tegra::Texture::ASTC
