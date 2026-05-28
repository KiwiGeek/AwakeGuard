#pragma once

#include <string>
#include <vector>

namespace paths {

constexpr wchar_t kAppFolderName[] = L"AwakeGuard-XP-LOL";
constexpr wchar_t kRunValueName[] = L"AwakeGuard-XP-LOL";
constexpr wchar_t kCleanupFlag[] = L"--cleanup-source";

std::wstring LocalAppDataDirectory();
std::wstring DataDirectory();
std::wstring SettingsFilePath();
std::wstring InstallDirectory();
std::wstring CurrentExecutablePath();

bool IsRunningFromInstallLocation();
bool IsTransientDirectory(const std::wstring& directory);
bool IsOnDesktopOrDownloads(const std::wstring& filePath = L"");

std::vector<std::wstring> RelocateArtifactPaths(const std::wstring& sourceExecutablePath);

std::wstring TryGetCleanupSourceExecutable(const std::vector<std::wstring>& args);
std::wstring FormatCleanupSourceArgument(const std::wstring& sourceExecutablePath);

}  // namespace paths
