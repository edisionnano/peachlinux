// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <mutex>
#include <string>
#include <vector>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#ifdef HAS_NSIGHT_AFTERMATH
#include <GFSDK_Aftermath_Defines.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>
#include <GFSDK_Aftermath_GpuCrashDumpDecoding.h>
#endif

#include "common/common_types.h"
#include "common/dynamic_library.h"

namespace Vulkan {

class NsightAftermathTracker {
public:
    NsightAftermathTracker();
    ~NsightAftermathTracker();

    NsightAftermathTracker(const NsightAftermathTracker&) = delete;
    NsightAftermathTracker& operator=(const NsightAftermathTracker&) = delete;

    // Delete move semantics because Aftermath initialization uses a pointer to this.
    NsightAftermathTracker(NsightAftermathTracker&&) = delete;
    NsightAftermathTracker& operator=(NsightAftermathTracker&&) = delete;

    bool Initialize();

    void SaveShader(const std::vector<u32>& spirv) const;

private:
#ifdef HAS_NSIGHT_AFTERMATH
    static void GpuCrashDumpCallback(const void* gpu_crash_dump, u32 gpu_crash_dump_size,
                                     void* user_data);

    static void ShaderDebugInfoCallback(const void* shader_debug_info, u32 shader_debug_info_size,
                                        void* user_data);

    static void CrashDumpDescriptionCallback(
        PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_description, void* user_data);

    void OnGpuCrashDumpCallback(const void* gpu_crash_dump, u32 gpu_crash_dump_size);

    void OnShaderDebugInfoCallback(const void* shader_debug_info, u32 shader_debug_info_size);

    void OnCrashDumpDescriptionCallback(
        PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription add_description);

    mutable std::mutex mutex;

    std::string dump_dir;
    int dump_id = 0;

    bool initialized = false;

    Common::DynamicLibrary dl;
    PFN_GFSDK_Aftermath_DisableGpuCrashDumps GFSDK_Aftermath_DisableGpuCrashDumps;
    PFN_GFSDK_Aftermath_EnableGpuCrashDumps GFSDK_Aftermath_EnableGpuCrashDumps;
    PFN_GFSDK_Aftermath_GetShaderDebugInfoIdentifier GFSDK_Aftermath_GetShaderDebugInfoIdentifier;
    PFN_GFSDK_Aftermath_GetShaderHashSpirv GFSDK_Aftermath_GetShaderHashSpirv;
    PFN_GFSDK_Aftermath_GpuCrashDump_CreateDecoder GFSDK_Aftermath_GpuCrashDump_CreateDecoder;
    PFN_GFSDK_Aftermath_GpuCrashDump_DestroyDecoder GFSDK_Aftermath_GpuCrashDump_DestroyDecoder;
    PFN_GFSDK_Aftermath_GpuCrashDump_GenerateJSON GFSDK_Aftermath_GpuCrashDump_GenerateJSON;
    PFN_GFSDK_Aftermath_GpuCrashDump_GetJSON GFSDK_Aftermath_GpuCrashDump_GetJSON;
#endif
};

#ifndef HAS_NSIGHT_AFTERMATH
inline NsightAftermathTracker::NsightAftermathTracker() = default;
inline NsightAftermathTracker::~NsightAftermathTracker() = default;
inline bool NsightAftermathTracker::Initialize() {
    return false;
}
inline void NsightAftermathTracker::SaveShader(const std::vector<u32>&) const {}
#endif

} // namespace Vulkan
