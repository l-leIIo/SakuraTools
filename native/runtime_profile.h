#pragma once

#include <string>

namespace sakura {

enum class MinecraftVersion {
    Auto,
    V1_8_9,
    V1_12_2,
    V1_16_5,
    V1_18_2,
    V1_20_1,
    V1_20_4,
    V1_21_0,
};

enum class ModLoader {
    Auto,
    Vanilla,
    Forge,
    NeoForge,
    Fabric,
    Quilt,
};

struct RuntimeProfile {
    MinecraftVersion version = MinecraftVersion::Auto;
    ModLoader loader = ModLoader::Auto;
    bool userOverride = false;
    std::string source = "default";
};

const char* ToString(MinecraftVersion version);
const char* ToString(ModLoader loader);

bool ParseVersion(const char* value, MinecraftVersion& version);
bool ParseLoader(const char* value, ModLoader& loader);

RuntimeProfile DefaultRuntimeProfile();
RuntimeProfile DetectRuntimeProfile();
RuntimeProfile DetectRuntimeProfileForMinecraftHome(const std::string& minecraftHome);
RuntimeProfile LoadRuntimeProfile();
void SaveRuntimeProfile(const RuntimeProfile& profile);

} // namespace sakura
