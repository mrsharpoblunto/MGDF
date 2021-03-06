#pragma once

#include <windows.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
#include <string>

namespace MGDF {
namespace core {

class Resources {
 public:
  static Resources *InstancePtr(HINSTANCE instance = nullptr) {
    static Resources r(instance);
    return &r;
  }

  static Resources &Instance(HINSTANCE instance = nullptr) {
    return *Resources::InstancePtr(instance);
  }

  std::wstring RootDir();

  void CreateRequiredDirectories();
  void SetUserBaseDir(bool useRootDir, const std::string &gameUid);
  void SetGameBaseDir(const std::wstring &gameDir);

  std::wstring GameBaseDir();
  std::wstring UserBaseDir();

  std::wstring ParamsFile();
  std::wstring GameFile();
  std::wstring WorkingDir();
  std::wstring SaveBaseDir();
  std::wstring GameStateSaveFile(const std::string &saveName);
  std::wstring SaveDir(const std::string &saveName);
  std::wstring SaveDataDir(const std::string &saveName);
  std::wstring CorePreferencesFile();
  std::wstring GameDefaultPreferencesFile();
  std::wstring GameUserPreferencesFile();
  std::wstring GameUserStatisticsFile();
  std::wstring ContentDir();
  std::wstring Module();
  std::wstring BinDir();
  std::wstring LogFile();

  static const UINT32 MIN_SCREEN_X;
  static const UINT32 MIN_SCREEN_Y;

  static std::wstring ToWString(const std::string &str);
  static std::wstring ToWString(const char *str);
  static std::string ToString(const std::wstring &str);
  static std::string ToString(const wchar_t *str);

 private:
  Resources(HINSTANCE instance);
  static std::wstring GetApplicationDirectory(HINSTANCE instance);
#pragma warning(push)
#pragma warning(disable : 4251)
  static std::wstring _vfsContent;
  std::wstring _applicationDirectory, _userBaseDir, _gameBaseDir;
#pragma warning(pop)
};

}  // namespace core
}  // namespace MGDF
