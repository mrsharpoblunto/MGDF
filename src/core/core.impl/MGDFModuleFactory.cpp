#include "StdAfx.h"

#pragma warning(push)
#pragma warning(disable : 4091)
#include <Dbghelp.h>
#pragma warning(pop)
#include <filesystem>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFCurrentDirectoryHelper.hpp"
#include "MGDFModuleFactory.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

// to enable the forced unloading of libraries once they are no longer in use by
// the application leaving this commented out leaves the unloading of the dlls
// to the OS NOTE: with FREE_UNUSED_LIBRARIES enabled memory leaks in external
// modules are not reported correctly #define FREE_UNUSED_LIBRARIES

using namespace std::filesystem;

namespace MGDF {
namespace core {

ModuleFactory::~ModuleFactory() {
#ifdef FREE_UNUSED_LIBRARIES
  if (_moduleInstance != nullptr) {
    FreeLibrary(_moduleInstance);
  }
#endif
}

HRESULT ModuleFactory::TryCreate(std::unique_ptr<ModuleFactory> &factory) {
  factory.reset();
  std::unique_ptr<ModuleFactory> f(new ModuleFactory());
  const auto result = f->Init();
  if (FAILED(result)) {
    return result;
  }
  factory.swap(f);
  return S_OK;
}

ModuleFactory::ModuleFactory()
    : _moduleInstance(nullptr),
      _getCustomArchiveHandlers(nullptr),
      _getModule(nullptr),
      _getCompatibleFeatureLevels(nullptr) {}

HRESULT ModuleFactory::Init() {
  path globalModule(Resources::Instance().Module());
  if (exists(globalModule)) {
    LOG("Loading Module.dll", MGDF_LOG_LOW);
    CurrentDirectoryHelper::Instance().Push(Resources::Instance().BinDir());
    _moduleInstance = LoadLibraryW(Resources::Instance().Module().c_str());
    CurrentDirectoryHelper::Instance().Pop();

    if (_moduleInstance != nullptr) {
      // required exported functions
      _getModule = (GetModulePtr)GetProcAddress(_moduleInstance, "GetModule");
      if (_getModule != nullptr) {
        LOG("Loaded GetModule from Module.dll", MGDF_LOG_LOW);
      } else {
        LOG("Module has no exported GetModule function", MGDF_LOG_ERROR);
        return E_FAIL;
      }

      // optional exported functions
      _getCustomArchiveHandlers = (GetCustomArchiveHandlersPtr)GetProcAddress(
          _moduleInstance, "GetCustomArchiveHandlers");
      if (_getCustomArchiveHandlers != nullptr) {
        LOG("Loaded GetCustomArchiveHandlers from Module.dll", MGDF_LOG_LOW);
      } else {
        LOG("Module has no exported GetCustomArchiveHandlers function",
            MGDF_LOG_LOW);
      }

      _getCompatibleFeatureLevels =
          (GetCompatibleFeatureLevelsPtr)GetProcAddress(
              _moduleInstance, "GetCompatibleFeatureLevels");
      if (_getCompatibleFeatureLevels != nullptr) {
        LOG("Loaded GetCompatibleFeatureLevels from Module.dll", MGDF_LOG_LOW);
      } else {
        LOG("Module has no exported GetCompatibleFeatureLevels function",
            MGDF_LOG_LOW);
      }

      return S_OK;
    } else {
      const DWORD errorCode = ::GetLastError();
      LOG("Failed to load Module.dll " << errorCode, MGDF_LOG_ERROR);

#if defined(_WIN64)
      constexpr bool win64 = true;
#else
      constexpr bool win64 = false;
#endif
      bool loggedMessage = false;

      HANDLE fileMapping = NULL;
      HANDLE file = CreateFileW(Resources::Instance().Module().c_str(),
                                GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
      if (file != INVALID_HANDLE_VALUE) {
        fileMapping = CreateFileMapping(file, NULL, PAGE_READONLY | SEC_IMAGE,
                                        0, 0, NULL);
        if (fileMapping != NULL) {
          LPVOID addressHeader =
              MapViewOfFileEx(fileMapping, FILE_MAP_READ, 0, 0, 0, NULL);
          if (addressHeader != NULL) {
            const PIMAGE_NT_HEADERS peHeader = ImageNtHeader(addressHeader);
            if (peHeader != NULL) {
              if (peHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 &&
                  win64) {
                LOG("Failed to load Module.dll - MGDF core is 64 bit and "
                    "Module is 32 "
                    "bit",
                    MGDF_LOG_ERROR);
                loggedMessage = true;
              } else if (peHeader->FileHeader.Machine ==
                             IMAGE_FILE_MACHINE_AMD64 &&
                         !win64) {
                LOG("Failed to load Module.dll - MGDF core is 32 bit and "
                    "Module is 64 "
                    "bit",
                    MGDF_LOG_ERROR);
                loggedMessage = true;
              }
            }
          }
        }
      }
      if (!loggedMessage) {
        LOG("Failed to load Module.dll - It doesn't appear to be a valid dll",
            MGDF_LOG_ERROR);
      }
      if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
      }
      if (fileMapping != NULL) {
        CloseHandle(fileMapping);
      }
    }
  }
  return E_FAIL;
}

HRESULT ModuleFactory::GetCustomArchiveHandlers(IMGDFArchiveHandler **list,
                                                UINT64 *length,
                                                IMGDFLogger *logger) const {
  if (_getCustomArchiveHandlers != nullptr) {
    return _getCustomArchiveHandlers(list, length, logger);
  } else {
    *length = 0;
    return S_OK;
  }
}

HRESULT ModuleFactory::GetModule(ComObject<IMGDFModule> &module) const {
  if (_getModule != nullptr) {
    return _getModule(module.Assign());
  } else {
    return E_FAIL;
  }
}

UINT64 ModuleFactory::GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                                 UINT64 *levelSize) const {
  if (_getCompatibleFeatureLevels != nullptr) {
    return _getCompatibleFeatureLevels(levels, levelSize);
  } else {
    return 0;
  }
}

bool ModuleFactory::GetLastError(std::string &error) const {
  error = _lastError;
  return _lastError.length() > 0;
}

}  // namespace core
}  // namespace MGDF