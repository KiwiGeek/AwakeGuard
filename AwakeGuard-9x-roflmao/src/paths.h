#pragma once

#include "str.h"

namespace paths {

constexpr char kAppFolderName[] = "AwakeGuard-9x-ROFLMAO";
constexpr char kRunValueName[] = "AwakeGuard-9x-ROFLMAO";
constexpr char kCleanupFlag[] = "--cleanup-source";

// The exe plus its .pdb if one sits next to it.
struct ArtifactList {
    PathStr items[2];
    int count;
};

PathStr UserDataRootDirectory();
PathStr DataDirectory();
PathStr SettingsFilePath();
PathStr InstallDirectory();
PathStr CurrentExecutablePath();

bool IsRunningFromInstallLocation();
bool IsTransientDirectory(const char* directory);
// Empty filePath means the running executable.
bool IsOnDesktopOrDownloads(const char* filePath = "");

ArtifactList RelocateArtifactPaths(const char* sourceExecutablePath);

// Parses GetCommandLineA() for --cleanup-source; empty if absent.
PathStr TryGetCleanupSourceExecutable();
TextStr FormatCleanupSourceArgument(const char* sourceExecutablePath);

}  // namespace paths
