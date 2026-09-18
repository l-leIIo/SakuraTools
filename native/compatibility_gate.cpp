#include "compatibility_gate.h"

namespace sakura {

CompatibilityDecision CheckCompatibility(const RuntimeProfile& profile) {
    // The current server/, login, forwarding and world_snapshot modules are
    // implemented for the 1.20.1 protocol family. Do not silently route other
    // profiles through those modern packet constructors.
    if (profile.version != MinecraftVersion::V1_20_1)
        return {false, "selected Minecraft version has no complete packet/lifecycle adapter"};
    if (profile.loader != ModLoader::Forge && profile.loader != ModLoader::Vanilla)
        return {false, "selected loader has no complete login/payload adapter"};
    return {true, "1.20.1 native protocol path"};
}

} // namespace sakura
