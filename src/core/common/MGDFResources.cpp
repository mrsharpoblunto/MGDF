#include "stdafx.h"

#include "MGDFResources.hpp"

#include <shlobj.h>
#include <zlib.h>

#include <filesystem>

#include "MGDFVersionInfo.hpp"

#define BUFFER_SIZE 4096

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace std::filesystem;

namespace MGDF {
namespace core {

template <typename T>
bool DecompressStringImpl(const char *str, const size_t strLen, T &out) {
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (inflateInit2(&zs, MAX_WBITS | 16) != Z_OK) {
    return false;
  }

  zs.next_in = (Bytef *)str;
  zs.avail_in = static_cast<uInt>(strLen);

  int ret = 0;
  do {
    out.resize(out.size() + BUFFER_SIZE);
    zs.next_out = (Bytef *)(out.data() + out.size() - BUFFER_SIZE);
    zs.avail_out = BUFFER_SIZE;

    ret = inflate(&zs, 0);
    out.resize(zs.total_out);
  } while (ret == Z_OK);

  inflateEnd(&zs);
  return ret == Z_STREAM_END;
}

bool Resources::CompressString(const std::string &str, std::vector<char> &out) {
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS | 16, 8,
                   Z_DEFAULT_STRATEGY)) {
    return false;
  }

  zs.next_in = (Byte *)str.data();
  zs.avail_in = static_cast<uInt>(str.size());

  int ret = 0;
  out.reserve(max(str.size(), BUFFER_SIZE));
  out.clear();
  do {
    out.resize(out.size() + BUFFER_SIZE);
    zs.next_out = (Bytef *)(out.data() + out.size() - BUFFER_SIZE);
    zs.avail_out = BUFFER_SIZE;

    ret = deflate(&zs, Z_FINISH);
    out.resize(zs.total_out);
  } while (ret == Z_OK);

  deflateEnd(&zs);
  return ret == Z_STREAM_END;
}

bool Resources::DecompressString(const char *str, const size_t strLen,
                                 std::string &out) {
  return DecompressStringImpl<std::string>(str, strLen, out);
}

bool Resources::DecompressString(const char *str, const size_t strLen,
                                 std::vector<char> &out) {
  return DecompressStringImpl<std::vector<char>>(str, strLen, out);
}

Resources::Resources(HINSTANCE instance) {
  if (instance != nullptr) {
    _applicationDirectory = GetApplicationDirectory(instance);
  } else {
    _applicationDirectory = L".";
  }
}

void Resources::SetUserBaseDir(const std::string &gameUid, bool useRootDir) {
  if (!useRootDir) {
    wchar_t strPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr,
                                   SHGFP_TYPE_CURRENT, strPath))) {
      std::wostringstream ss;
      ss << strPath << L"/MGDF/" << MGDFVersionInfo::MGDF_INTERFACE_VERSION
         << L"/" << ToWString(gameUid) << (!gameUid.empty() ? L"/" : L"");
      _userBaseDir = ss.str();
      return;
    }
  }

  if (!_gameBaseDir.empty()) {
    std::filesystem::path gamesDirPath(_gameBaseDir);
    _userBaseDir =
        gamesDirPath.parent_path().parent_path().wstring() + L"/user/";
  } else {
    _userBaseDir = _applicationDirectory + L"user/" + ToWString(gameUid) +
                   (!gameUid.empty() ? L"/" : L"");
  }
}

/**
get the directory the application is contained within
*/
std::wstring Resources::GetApplicationDirectory(HINSTANCE instance) {
  // get the application directory
  wchar_t *pStr, szPath[MAX_PATH];
  GetModuleFileNameW(instance, szPath, MAX_PATH);
  pStr = wcsrchr(szPath, L'\\');
  for (wchar_t *chr = pStr; chr >= &szPath[0]; --chr) {
    if (*chr == L'\\') {
      *chr = L'/';
    }
  }
  if (pStr != nullptr) *(++pStr) = L'\0';

  std::wstring appDir = szPath;
  return appDir;
}

const UINT32 Resources::MIN_SCREEN_X = 1024;
const UINT32 Resources::MIN_SCREEN_Y = 768;

std::wstring Resources::LogFile() { return UserBaseDir() + L"coreLog.txt"; }

std::wstring Resources::ParamsFile() { return RootDir() + L"params.txt"; }

std::wstring Resources::RootDir() { return _applicationDirectory; }

std::wstring Resources::UserBaseDir() { return _userBaseDir; }

void Resources::SetGameBaseDir(const std::wstring &gameDir) {
  _gameBaseDir = gameDir;
}

std::wstring Resources::GameBaseDir() {
  return !_gameBaseDir.empty() ? _gameBaseDir
                               : (_applicationDirectory + L"game/");
}

std::wstring Resources::GameFile() { return GameBaseDir() + L"game.json"; }

std::wstring Resources::WorkingDir() { return UserBaseDir() + L"working/"; }

std::wstring Resources::SaveBaseDir() { return UserBaseDir() + L"saves/"; }

std::wstring Resources::GameStateSaveFile(const std::string &saveName) {
  return SaveDir(saveName) + L"gameState.json";
}

std::wstring Resources::SaveDir(const std::string &saveName) {
  return SaveBaseDir() + ToWString(saveName) + L"/";
}

std::wstring Resources::SaveDataDir(const std::string &saveName) {
  return SaveDir(saveName) + L"data/";
}

void Resources::CreateRequiredDirectories() {
  std::filesystem::path userBaseDir(UserBaseDir());
  create_directories(userBaseDir);

  std::filesystem::path saveBaseDir(SaveBaseDir());
  create_directories(saveBaseDir);

  std::filesystem::path workingDir(WorkingDir());
  create_directories(workingDir);
}

std::wstring Resources::CorePreferencesFile() {
  return _applicationDirectory + L"resources/preferences.json";
}

std::wstring Resources::GameDefaultPreferencesFile() {
  return GameBaseDir() + L"preferences.json";
}

std::wstring Resources::GameUserPreferencesFile() {
  return UserBaseDir() + L"preferences.json";
}

std::wstring Resources::GameUserStatisticsFile() {
  return UserBaseDir() + L"statistics.txt";
}

std::wstring Resources::ContentDir() { return GameBaseDir() + L"content/"; }

std::wstring Resources::Module() { return BinDir() + L"module.dll"; }

std::wstring Resources::BinDir() { return GameBaseDir() + L"bin/"; }

std::string Resources::ToString(const std::wstring &wstr) {
  const INT32 sizeNeeded = WideCharToMultiByte(
      CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);

  std::string result;
  result.resize(sizeNeeded);

  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), result.data(),
                      sizeNeeded, nullptr, nullptr);
  return result;
}

std::string Resources::ToString(const wchar_t *wstr) {
  _ASSERTE(wstr);
  if (!wstr) return "";

  const size_t len = wcslen(wstr);
  const INT32 sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)len,
                                               nullptr, 0, nullptr, nullptr);

  std::string result;
  result.resize(sizeNeeded);

  WideCharToMultiByte(CP_UTF8, 0, wstr, (int)len, result.data(), sizeNeeded,
                      nullptr, nullptr);
  return result;
}

std::wstring Resources::ToWString(const std::string &str) {
  const INT32 sizeNeeded =
      MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);

  std::wstring result;
  result.resize(sizeNeeded);

  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), result.data(),
                      sizeNeeded);
  return result;
}

std::wstring Resources::ToWString(const char *str) {
  _ASSERTE(str);
  if (!str) return L"";

  const size_t len = strlen(str);
  const INT32 sizeNeeded =
      MultiByteToWideChar(CP_UTF8, 0, str, (int)len, nullptr, 0);

  std::wstring result;
  result.resize(sizeNeeded);

  MultiByteToWideChar(CP_UTF8, 0, str, (int)len, result.data(), sizeNeeded);
  return result;
}

}  // namespace core
}  // namespace MGDF