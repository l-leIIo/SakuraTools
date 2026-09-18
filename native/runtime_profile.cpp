#include "runtime_profile.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <windows.h>

namespace {

std::string Trim(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
        value.erase(value.begin());
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
        value.pop_back();
    return value;
}

std::string LowerCopy(const std::string& value) {
    std::string out = value;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

bool PathExists(const std::string& path) {
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES;
}

std::string JoinPath(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    if (a.back() == '\\' || a.back() == '/')
        return a + b;
    return a + "\\" + b;
}

int VersionRank(sakura::MinecraftVersion version) {
    switch (version) {
    case sakura::MinecraftVersion::V1_8_9: return 8;
    case sakura::MinecraftVersion::V1_12_2: return 12;
    case sakura::MinecraftVersion::V1_16_5: return 16;
    case sakura::MinecraftVersion::V1_18_2: return 18;
    case sakura::MinecraftVersion::V1_20_1: return 20;
    case sakura::MinecraftVersion::V1_20_4: return 20;
    case sakura::MinecraftVersion::V1_21_0: return 21;
    default: return 0;
    }
}

sakura::MinecraftVersion ParseVersionFromText(const std::string& text) {
    std::string lowered = LowerCopy(text);
    if (lowered.find("1.21") != std::string::npos || lowered.find("1.21.0") != std::string::npos)
        return sakura::MinecraftVersion::V1_21_0;
    if (lowered.find("1.20.4") != std::string::npos || lowered.find("1.20.4") != std::string::npos)
        return sakura::MinecraftVersion::V1_20_4;
    if (lowered.find("1.20.1") != std::string::npos || lowered.find("1.20") != std::string::npos)
        return sakura::MinecraftVersion::V1_20_1;
    if (lowered.find("1.18.2") != std::string::npos || lowered.find("1.18") != std::string::npos)
        return sakura::MinecraftVersion::V1_18_2;
    if (lowered.find("1.16.5") != std::string::npos || lowered.find("1.16") != std::string::npos)
        return sakura::MinecraftVersion::V1_16_5;
    if (lowered.find("1.12.2") != std::string::npos || lowered.find("1.12") != std::string::npos)
        return sakura::MinecraftVersion::V1_12_2;
    if (lowered.find("1.8.9") != std::string::npos || lowered.find("1.8") != std::string::npos)
        return sakura::MinecraftVersion::V1_8_9;
    return sakura::MinecraftVersion::Auto;
}

sakura::ModLoader DetectLoaderFromText(const std::string& text) {
    std::string lowered = LowerCopy(text);
    if (lowered.find("neoforge") != std::string::npos)
        return sakura::ModLoader::NeoForge;
    if (lowered.find("forge") != std::string::npos)
        return sakura::ModLoader::Forge;
    if (lowered.find("quilt") != std::string::npos)
        return sakura::ModLoader::Quilt;
    if (lowered.find("fabric") != std::string::npos)
        return sakura::ModLoader::Fabric;
    if (lowered.find("vanilla") != std::string::npos || lowered.find("official") != std::string::npos)
        return sakura::ModLoader::Vanilla;
    return sakura::ModLoader::Auto;
}

std::string ReadTextFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open())
        return {};
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

bool CreateDirectoryRecursive(const std::string& path) {
    std::string current;
    std::string normalized = path;
    if (normalized.size() > 1 && normalized[1] == ':')
        current = normalized.substr(0, 2);
    else if (!normalized.empty() && normalized[0] == '\\')
        current = "\\\\";
    std::string rest = normalized;
    if (normalized.size() > 2 && normalized[2] == '\\')
        rest = normalized.substr(3);
    std::istringstream stream(rest);
    std::string part;
    while (std::getline(stream, part, '\\')) {
        if (part.empty())
            continue;
        current = current.empty() ? part : (current + "\\" + part);
        if (!PathExists(current)) {
            if (!CreateDirectoryA(current.c_str(), nullptr)) {
                DWORD err = GetLastError();
                if (err != ERROR_ALREADY_EXISTS)
                    return false;
            }
        }
    }
    return true;
}

} // namespace

namespace sakura {

const char* ToString(MinecraftVersion version) {
    switch (version) {
    case MinecraftVersion::Auto: return "auto";
    case MinecraftVersion::V1_8_9: return "1.8.9";
    case MinecraftVersion::V1_12_2: return "1.12.2";
    case MinecraftVersion::V1_16_5: return "1.16.5";
    case MinecraftVersion::V1_18_2: return "1.18.2";
    case MinecraftVersion::V1_20_1: return "1.20.1";
    case MinecraftVersion::V1_20_4: return "1.20.4";
    case MinecraftVersion::V1_21_0: return "1.21.0";
    default: return "unknown";
    }
}

const char* ToString(ModLoader loader) {
    switch (loader) {
    case ModLoader::Auto: return "auto";
    case ModLoader::Vanilla: return "vanilla";
    case ModLoader::Forge: return "forge";
    case ModLoader::NeoForge: return "neoforge";
    case ModLoader::Fabric: return "fabric";
    case ModLoader::Quilt: return "quilt";
    default: return "unknown";
    }
}

bool ParseVersion(const char* value, MinecraftVersion& version) {
    if (!value || !*value)
        return false;
    std::string lower = LowerCopy(value);
    if (lower == "auto" || lower == "default") {
        version = MinecraftVersion::Auto;
        return true;
    }
    if (lower == "1.8.9" || lower == "1.8") {
        version = MinecraftVersion::V1_8_9;
        return true;
    }
    if (lower == "1.12.2" || lower == "1.12") {
        version = MinecraftVersion::V1_12_2;
        return true;
    }
    if (lower == "1.16.5" || lower == "1.16") {
        version = MinecraftVersion::V1_16_5;
        return true;
    }
    if (lower == "1.18.2" || lower == "1.18") {
        version = MinecraftVersion::V1_18_2;
        return true;
    }
    if (lower == "1.20.1" || lower == "1.20") {
        version = MinecraftVersion::V1_20_1;
        return true;
    }
    if (lower == "1.20.4") {
        version = MinecraftVersion::V1_20_4;
        return true;
    }
    if (lower == "1.21.0" || lower == "1.21") {
        version = MinecraftVersion::V1_21_0;
        return true;
    }
    return false;
}

bool ParseLoader(const char* value, ModLoader& loader) {
    if (!value || !*value)
        return false;
    std::string lower = LowerCopy(value);
    if (lower == "auto" || lower == "default") {
        loader = ModLoader::Auto;
        return true;
    }
    if (lower == "vanilla") {
        loader = ModLoader::Vanilla;
        return true;
    }
    if (lower == "forge") {
        loader = ModLoader::Forge;
        return true;
    }
    if (lower == "neoforge") {
        loader = ModLoader::NeoForge;
        return true;
    }
    if (lower == "fabric") {
        loader = ModLoader::Fabric;
        return true;
    }
    if (lower == "quilt") {
        loader = ModLoader::Quilt;
        return true;
    }
    return false;
}

RuntimeProfile DefaultRuntimeProfile() {
    RuntimeProfile profile;
    profile.version = MinecraftVersion::V1_20_1;
    profile.loader = ModLoader::Forge;
    profile.userOverride = false;
    profile.source = "default";
    return profile;
}

RuntimeProfile DetectRuntimeProfileForMinecraftHome(const std::string& minecraftHome) {
    RuntimeProfile best = DefaultRuntimeProfile();
    std::string versionsRoot = JoinPath(minecraftHome, "versions");
    if (!PathExists(versionsRoot))
        return best;

    std::string pattern = JoinPath(versionsRoot, "*");
    WIN32_FIND_DATAA findData{};
    HANDLE handle = FindFirstFileA(pattern.c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE)
        return best;

    do {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            continue;
        if (std::string(findData.cFileName) == "." || std::string(findData.cFileName) == "..")
            continue;

        std::string versionDir = JoinPath(versionsRoot, findData.cFileName);
        std::string versionJson = JoinPath(versionDir, std::string(findData.cFileName) + ".json");
        std::string loaderText = {};
        if (PathExists(versionJson)) {
            loaderText = ReadTextFile(versionJson);
        }
        if (loaderText.empty()) {
            std::string manifest = JoinPath(versionDir, "version.json");
            if (PathExists(manifest))
                loaderText = ReadTextFile(manifest);
        }
        if (loaderText.empty())
            continue;

        RuntimeProfile candidate = best;
        candidate.version = ParseVersionFromText(findData.cFileName);
        candidate.loader = DetectLoaderFromText(loaderText);
        if (candidate.loader == ModLoader::Auto)
            candidate.loader = ModLoader::Vanilla;
        if (candidate.version == MinecraftVersion::Auto)
            candidate.version = ParseVersionFromText(loaderText);
        if (candidate.version == MinecraftVersion::Auto)
            continue;

        if (VersionRank(candidate.version) > VersionRank(best.version)) {
            best = candidate;
            best.source = "minecraft-home-detect";
        }
    } while (FindNextFileA(handle, &findData));

    FindClose(handle);
    if (best.version == MinecraftVersion::Auto)
        return DefaultRuntimeProfile();
    return best;
}

RuntimeProfile DetectRuntimeProfile() {
    char appData[MAX_PATH] = {};
    DWORD len = GetEnvironmentVariableA("APPDATA", appData, sizeof(appData));
    if (len > 0 && len < sizeof(appData)) {
        RuntimeProfile detected = DetectRuntimeProfileForMinecraftHome(std::string(appData) + "\\.minecraft");
        if (detected.version != MinecraftVersion::Auto)
            return detected;
    }
    return DefaultRuntimeProfile();
}

RuntimeProfile LoadRuntimeProfile() {
    char appData[MAX_PATH] = {};
    DWORD len = GetEnvironmentVariableA("APPDATA", appData, sizeof(appData));
    if (len == 0 || len >= sizeof(appData))
        return DetectRuntimeProfile();

    std::string profileDir = std::string(appData) + "\\SakuraTools";
    std::string profilePath = profileDir + "\\runtime_profile.json";
    if (!PathExists(profilePath))
        return DetectRuntimeProfile();

    std::string text = ReadTextFile(profilePath);
    std::string versionValue = "1.20.1";
    std::string loaderValue = "forge";

    std::string::size_type pos = 0;
    for (std::string token : {"\"version\"", "\"loader\"", "\"user_override\""}) {
        pos = text.find(token);
        if (pos == std::string::npos)
            continue;
        pos = text.find(':', pos);
        if (pos == std::string::npos)
            continue;
        pos += 1;
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
            pos += 1;
        size_t end = pos;
        while (end < text.size() && text[end] != ',' && text[end] != '}' && text[end] != '\n' && text[end] != '\r')
            end += 1;
        std::string value = Trim(text.substr(pos, end - pos));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        if (token == "\"version\"")
            versionValue = value;
        else if (token == "\"loader\"")
            loaderValue = value;
    }

    RuntimeProfile profile = DefaultRuntimeProfile();
    if (ParseVersion(versionValue.c_str(), profile.version))
        profile.source = "user-profile";
    if (ParseLoader(loaderValue.c_str(), profile.loader))
        profile.source = "user-profile";
    profile.userOverride = true;
    return profile;
}

void SaveRuntimeProfile(const RuntimeProfile& profile) {
    char appData[MAX_PATH] = {};
    DWORD len = GetEnvironmentVariableA("APPDATA", appData, sizeof(appData));
    if (len == 0 || len >= sizeof(appData))
        return;

    std::string dir = std::string(appData) + "\\SakuraTools";
    if (!CreateDirectoryRecursive(dir))
        return;

    std::string path = dir + "\\runtime_profile.json";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return;
    out << "{\n"
        << "  \"version\": \"" << ToString(profile.version) << "\",\n"
        << "  \"loader\": \"" << ToString(profile.loader) << "\",\n"
        << "  \"user_override\": " << (profile.userOverride ? "true" : "false") << "\n"
        << "}\n";
    out.close();
}

} // namespace sakura
