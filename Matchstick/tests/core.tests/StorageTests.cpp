#include "stdafx.h"

#include <filesystem>

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/common/MGDFVersionHelper.hpp"
#include "../../src/core/storage/jsoncpp/JsonCppStorageFactoryComponent.hpp"
#include "../../src/core/vfs/MGDFReadOnlyVirtualFileSystemComponent.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;
using namespace MGDF::core::storage;
using namespace std::filesystem;

SUITE(StorageTests) {
  struct StorageTestFixture {
   public:
    StorageTestFixture() {
      HINSTANCE inst;
      inst = (HINSTANCE)GetModuleHandleW(L"core.tests.exe");
      Resources::Instance(inst);
      Resources::Instance().SetUserBaseDir("junkship", true);

      CreateReadOnlyVirtualFileSystemComponent(_vfs);
      _vfs->Mount((Resources::Instance().RootDir() + L"../../../tests/content")
                      .c_str());
      jsoncppImpl::CreateStorageFactoryComponent(_storage);
    }
    virtual ~StorageTestFixture() {}

   protected:
    ComObject<IReadOnlyVirtualFileSystemComponent> _vfs;
    std::shared_ptr<IStorageFactoryComponent> _storage;
  };

  /**
  ensure that game files can be read by the engine
  */
  TEST_FIXTURE(StorageTestFixture, StorageGameHandlerTest) {
    std::unique_ptr<IGameStorageHandler> handler(
        _storage->CreateGameStorageHandler());

    ComObject<IMGDFReadOnlyFile> file;
    CHECK(_vfs->GetFile(L"console.json", file.Assign()));
    std::wstring path = ComString<&IMGDFReadOnlyFile::GetPhysicalPath>(file);
    handler->Load(path);

    CHECK_EQUAL("Console", handler->GetGameUid());
    CHECK_EQUAL("Lua Console", handler->GetGameName());
    MGDFVersion expected;
    expected.Major = 0;
    expected.Minor = 1;
    expected.Build = -1;
    expected.Revision = -1;
    MGDFVersion v;
    handler->GetVersion(v);
    CHECK_EQUAL(0, VersionHelper::Compare(v, expected));
  }

  /**
  ensure that game state files can be read by the engine
  */
  TEST_FIXTURE(StorageTestFixture, StorageGameStateHandlerTest) {
    MGDFVersion expected = VersionHelper::Create("0.1");
    CHECK_EQUAL("0.1", VersionHelper::Format(expected));
    CHECK_EQUAL(0,
                VersionHelper::Compare(expected, VersionHelper::Create("0.1")));
    CHECK_EQUAL(
        -1, VersionHelper::Compare(expected, VersionHelper::Create("0.1.1")));
    CHECK_EQUAL(
        1, VersionHelper::Compare(expected, VersionHelper::Create("0.0.1")));

    auto handler = _storage->CreateGameStateStorageHandler("Console", expected);

    ComObject<IMGDFReadOnlyFile> file;
    CHECK(_vfs->GetFile(L"gameState.json", file.Assign()));
    std::wstring path = ComString<&IMGDFReadOnlyFile::GetPhysicalPath>(file);
    handler->Load(path);

    CHECK_EQUAL("Console", handler->GetGameUid());
    MGDFVersion v;
    handler->GetVersion(v);
    CHECK_EQUAL(0, VersionHelper::Compare(v, expected));

    std::wstring savePath =
        Resources::Instance().RootDir() + L"../../../tests/content/temp.json";
    handler->Save(savePath);

    // reload using the freshly saved file, the contents should not have changed
    handler = _storage->CreateGameStateStorageHandler("Console", expected);
    handler->Load(savePath);

    CHECK_EQUAL("Console", handler->GetGameUid());
    handler->GetVersion(v);
    CHECK_EQUAL(0, VersionHelper::Compare(v, expected));

    remove(std::filesystem::path(savePath));  // remove the temp file
  }

  /**
  ensure that preferences can be loaded and saved by the engine
  */
  TEST_FIXTURE(StorageTestFixture, StoragePreferencesHandlerTest) {
    auto handler = _storage->CreatePreferenceConfigStorageHandler();

    ComObject<IMGDFReadOnlyFile> file;
    CHECK(_vfs->GetFile(L"preferences.json", file.Assign()));
    std::wstring path = ComString<&IMGDFReadOnlyFile::GetPhysicalPath>(file);
    handler->Load(path);

    INT32 count = 0;
    bool foundResolution = false;
    bool foundScreenX = false;
    for (auto pref : *handler) {
      if (pref.first == "resolution") {
        CHECK_EQUAL("800*600", pref.second);
        foundResolution = true;
      } else if (pref.first == "host.screenX") {
        CHECK_EQUAL("800", pref.second);
        foundScreenX = true;
      }
      ++count;
    }
    CHECK(foundResolution);
    CHECK(foundScreenX);
    CHECK_EQUAL(9, count);

    std::wstring savePath =
        Resources::Instance().RootDir() + L"../../../tests/content/temp.json";
    if (exists(std::filesystem::path(savePath))) {
      remove(std::filesystem::path(savePath));  // remove the temp file
    }
    handler->Save(savePath);

    // reload the file, it should be identical
    handler = _storage->CreatePreferenceConfigStorageHandler();
    handler->Load(path);

    count = 0;
    foundResolution = false;
    foundScreenX = false;
    for (auto pref : *handler) {
      if (pref.first == "resolution") {
        CHECK_EQUAL("800*600", pref.second);
        foundResolution = true;
      } else if (pref.first == "host.screenX") {
        CHECK_EQUAL("800", pref.second);
        foundScreenX = true;
      }
      ++count;
    }
    CHECK(foundResolution);
    CHECK(foundScreenX);
    CHECK_EQUAL(9, count);

    remove(std::filesystem::path(savePath));  // remove the temp file
  }
}