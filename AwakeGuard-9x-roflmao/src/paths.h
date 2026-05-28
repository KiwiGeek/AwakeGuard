#pragma once

#include <string>
#include <vector>

namespace paths {

constexpr char kAppFolderName[] = "AwakeGuard-9x-ROFLMAO";
constexpr char kRunValueName[] = "AwakeGuard-9x-ROFLMAO";
constexpr char kCleanupFlag[] = "--cleanup-source";

std::string UserDataRootDirectory();
std::string DataDirectory();
std::string SettingsFilePath();
std::string InstallDirectory();
std::string CurrentExecutablePath();

bool IsRunningFromInstallLocation();
bool IsTransientDirectory(const std::string& directory);
bool IsOnDesktopOrDownloads(const std::string& filePath = "");

std::vector<std::string> RelocateArtifactPaths(const std::string& sourceExecutablePath);

std::string TryGetCleanupSourceExecutable(const std::vector<std::string>& args);
std::string FormatCleanupSourceArgument(const std::string& sourceExecutablePath);

}  // namespace paths
