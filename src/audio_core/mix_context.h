// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>
#include "audio_core/common.h"
#include "audio_core/splitter_context.h"
#include "common/common_funcs.h"
#include "common/common_types.h"

namespace AudioCore {
class BehaviorInfo;

class MixInfo {
public:
    struct DirtyHeader {
        u32_le magic{};
        u32_le mixer_count{};
        INSERT_PADDING_BYTES(0x18);
    };
    static_assert(sizeof(DirtyHeader) == 0x20, "MixInfo::DirtyHeader is an invalid size");

    struct InParams {
        float_le volume{};
        s32_le sample_rate{};
        s32_le buffer_count{};
        bool in_use{};
        INSERT_PADDING_BYTES(3);
        s32_le mix_id{};
        s32_le effect_count{};
        u32_le node_id{};
        INSERT_PADDING_WORDS(2);
        std::array<std::array<float_le, AudioCommon::MAX_MIX_BUFFERS>, AudioCommon::MAX_MIX_BUFFERS>
            mix_volume{};
        s32_le dest_mix_id{};
        s32_le splitter_id{};
        INSERT_PADDING_WORDS(1);
    };
    static_assert(sizeof(MixInfo::InParams) == 0x930, "MixInfo::InParams is an invalid size");
};

class ServerMixInfo {
public:
    struct InParams {
        float volume{};
        s32 sample_rate{};
        s32 buffer_count{};
        bool in_use{};
        s32 mix_id{};
        u32 node_id{};
        std::array<std::array<float_le, AudioCommon::MAX_MIX_BUFFERS>, AudioCommon::MAX_MIX_BUFFERS>
            mix_volume{};
        s32 dest_mix_id{};
        s32 splitter_id{};
        s32 buffer_offset{};
        s32 final_mix_distance{};
    };
    ServerMixInfo();
    ~ServerMixInfo();

    const ServerMixInfo::InParams& GetInParams() const;
    ServerMixInfo::InParams& GetInParams();

    bool Update(EdgeMatrix& edge_matrix, const MixInfo::InParams& mix_in,
                BehaviorInfo& behavior_info, SplitterContext& splitter_context);
    bool HasAnyConnection() const;
    void Cleanup();

private:
    InParams in_params{};
    bool UpdateConnection(EdgeMatrix& edge_matrix, const MixInfo::InParams& mix_in,
                          SplitterContext& splitter_context);
};

class MixContext {
public:
    MixContext();
    ~MixContext();

    void Initialize(const BehaviorInfo& behavior_info, std::size_t mix_count);
    void SortInfo();
    bool TsortInfo(SplitterContext& splitter_context);

    std::size_t GetCount() const;
    ServerMixInfo& GetInfo(std::size_t i);
    const ServerMixInfo& GetInfo(std::size_t i) const;
    ServerMixInfo& GetSortedInfo(std::size_t i);
    const ServerMixInfo& GetSortedInfo(std::size_t i) const;
    ServerMixInfo& GetFinalMixInfo();
    const ServerMixInfo& GetFinalMixInfo() const;
    EdgeMatrix& GetEdgeMatrix();
    const EdgeMatrix& GetEdgeMatrix() const;

private:
    void CalcMixBufferOffset();
    void UpdateDistancesFromFinalMix();

    NodeStates node_states{};
    EdgeMatrix edge_matrix{};
    std::size_t info_count{};
    std::vector<ServerMixInfo> infos{};
    std::vector<ServerMixInfo*> sorted_info{};
};
} // namespace AudioCore
