#pragma once

#include <string>

namespace sakura {

enum class MinecraftVersion {
    Auto,
    V1_8_0,
    V1_8_8,
    V1_8_9,
    V1_9_4,
    V1_10_2,
    V1_11_2,
    V1_12_2,
    V1_13_2,
    V1_14_4,
    V1_15_2,
    V1_16_5,
    V1_17_1,
    V1_18_2,
    V1_18_3,
    V1_19_2,
    V1_19_4,
    V1_20_1,
    V1_20_2,
    V1_20_4,
    V1_20_6,
    V1_21_0,
    V1_21_1,
    V1_21_3,
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
