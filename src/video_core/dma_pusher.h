// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>
#include <queue>

#include "common/bit_field.h"
#include "common/common_types.h"
#include "video_core/engines/engine_interface.h"

namespace Core {
class System;
}

namespace Tegra {

enum class SubmissionMode : u32 {
    IncreasingOld = 0,
    Increasing = 1,
    NonIncreasingOld = 2,
    NonIncreasing = 3,
    Inline = 4,
    IncreaseOnce = 5
};

struct CommandListHeader {
    union {
        u64 raw;
        BitField<0, 40, GPUVAddr> addr;
        BitField<41, 1, u64> is_non_main;
        BitField<42, 21, u64> size;
    };
};
static_assert(sizeof(CommandListHeader) == sizeof(u64), "CommandListHeader is incorrect size");

union CommandHeader {
    u32 argument;
    BitField<0, 13, u32> method;
    BitField<0, 24, u32> method_count_;
    BitField<13, 3, u32> subchannel;
    BitField<16, 13, u32> arg_count;
    BitField<16, 13, u32> method_count;
    BitField<29, 3, SubmissionMode> mode;
};
static_assert(std::is_standard_layout_v<CommandHeader>, "CommandHeader is not standard layout");
static_assert(sizeof(CommandHeader) == sizeof(u32), "CommandHeader has incorrect size!");

class GPU;

using CommandList = std::vector<Tegra::CommandListHeader>;

/**
 * The DmaPusher class implements DMA submission to FIFOs, providing an area of memory that the
 * emulated app fills with commands and tells PFIFO to process. The pushbuffers are then assembled
 * into a "command stream" consisting of 32-bit words that make up "commands".
 * See https://envytools.readthedocs.io/en/latest/hw/fifo/dma-pusher.html#fifo-dma-pusher for
 * details on this implementation.
 */
class DmaPusher {
public:
    explicit DmaPusher(Core::System& system, GPU& gpu);
    ~DmaPusher();

    void Push(CommandList&& entries) {
        dma_pushbuffer.push(std::move(entries));
    }

    void DispatchCalls();

    void BindSubchannel(Tegra::Engines::EngineInterface* engine, u32 subchannel_id) {
        subchannels[subchannel_id] = engine;
    }

private:
    static constexpr u32 non_puller_methods = 0x40;
    static constexpr u32 max_subchannels = 8;
    bool Step();

    void SetState(const CommandHeader& command_header);

    void CallMethod(u32 argument) const;
    void CallMultiMethod(const u32* base_start, u32 num_methods) const;

    std::vector<CommandHeader> command_headers; ///< Buffer for list of commands fetched at once

    std::queue<CommandList> dma_pushbuffer; ///< Queue of command lists to be processed
    std::size_t dma_pushbuffer_subindex{};  ///< Index within a command list within the pushbuffer

    struct DmaState {
        u32 method;            ///< Current method
        u32 subchannel;        ///< Current subchannel
        u32 method_count;      ///< Current method count
        u32 length_pending;    ///< Large NI command length pending
        bool non_incrementing; ///< Current command's NI flag
        bool is_last_call;
    };

    DmaState dma_state{};
    bool dma_increment_once{};

    bool ib_enable{true}; ///< IB mode enabled

    std::array<Tegra::Engines::EngineInterface*, max_subchannels> subchannels{};

    GPU& gpu;
    Core::System& system;
};

} // namespace Tegra