#include "stdafx.h"

#include <filesystem>
#include <fstream>

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/core.impl/MGDFSaveManagerImpl.hpp"
#include "../../src/core/storage/jsoncpp/JsonStorageFactoryComponent.hpp"

using namespace MGDF;
using namespace MGDF::core;
namespace fs = std::filesystem;

class SaveManagerTests : public ::testing::Test {
 protected:
  void SetUp() override {
    Resources::Instance(GetModuleHandleW(L"core.tests.exe"));
    std::string name;
    ASSERT_TRUE(GuidString(name));
    Resources::Instance().SetUserBaseDir("save-tests-" + name, true);
    _root = Resources::Instance().UserBaseDir();
    fs::create_directories(Resources::Instance().SaveBaseDir());
    vfs::CreateReadOnlyVirtualFileSystemComponent(_vfs);
    storage::json::CreateStorageFactoryComponent(_storage);
    MGDFVersion version = {};
    auto game =
        MakeCom<Game>("save-tests", "Save tests", "", version, _storage);
    _saves = MakeCom<SaveManager>(game, _vfs, _storage);
  }

  void TearDown() override {
    _saves.Clear();
    std::error_code error;
    fs::remove_all(_root, error);
    Resources::Instance().SetUserBaseDir("junkship", true);
  }

  ComObject<GameState> ExistingSave() {
    const fs::path root(Resources::Instance().SaveDir("existing"));
    fs::create_directories(root / "data");
    std::ofstream(root / "data" / "value.txt") << "old";
    return MakeCom<GameState>("existing", "save-tests", _saves.Get(), _vfs,
                              _storage);
  }

  static fs::path PendingPath(IMGDFWriteableVirtualFileSystem *pending) {
    ComObject<IMGDFWriteableFile> root;
    pending->GetRoot(root.Assign());
    return std::wstring(ComString<&IMGDFWriteableFile::GetPhysicalPath>(root));
  }

  static std::string Read(const fs::path &file) {
    std::ifstream input(file);
    return std::string(std::istreambuf_iterator<char>(input), {});
  }

  fs::path _root;
  ComObject<vfs::IReadOnlyVirtualFileSystemComponent> _vfs;
  std::shared_ptr<storage::IStorageFactoryComponent> _storage;
  ComObject<SaveManager> _saves;
};

TEST_F(SaveManagerTests, NewSaveCommitsOnRelease) {
  ComObject<IMGDFGameState> state;
  _saves->CreateGameState(state.Assign());
  ComObject<IMGDFWriteableVirtualFileSystem> pending;
  ASSERT_EQ(S_OK, state->BeginSave(pending.Assign(), nullptr));
  std::ofstream(PendingPath(pending) / "value.txt") << "new";
  pending.Clear();
  ASSERT_FALSE(state->IsNew());
  ASSERT_EQ(1u, _saves->GetSaveCount());
  ComObject<IMGDFGameState> loaded;
  ASSERT_EQ(S_OK, _saves->GetSave(0, loaded.Assign()));
  ComObject<IMGDFReadOnlyVirtualFileSystem> saved;
  ASSERT_EQ(S_OK, loaded->GetVFS(saved.Assign()));
  ComObject<IMGDFReadOnlyFile> file;
  ASSERT_TRUE(saved->GetFile(L"value.txt", file.Assign()));
  EXPECT_EQ(
      "new",
      Read(std::wstring(ComString<&IMGDFReadOnlyFile::GetPhysicalPath>(file))));
}

TEST_F(SaveManagerTests, ReplacementPreservesPreviousUntilRelease) {
  auto state = ExistingSave();
  ComObject<IMGDFWriteableVirtualFileSystem> pending;
  ComObject<IMGDFReadOnlyVirtualFileSystem> previous;
  ASSERT_EQ(S_OK, state->BeginSave(pending.Assign(), previous.Assign()));
  ASSERT_TRUE(previous);
  const fs::path root(Resources::Instance().SaveDir("existing"));
  std::ofstream(PendingPath(pending) / "value.txt") << "new";
  EXPECT_EQ("old", Read(root / "data" / "value.txt"));
  pending.Clear();
  EXPECT_EQ("new", Read(root / "data" / "value.txt"));
  EXPECT_FALSE(fs::exists(root / "backup"));
}

TEST_F(SaveManagerTests, MissingPreviousDataDoesNotCreatePendingSave) {
  auto state = ExistingSave();
  fs::remove_all(fs::path(Resources::Instance().SaveDir("existing")) / "data");
  ComObject<IMGDFWriteableVirtualFileSystem> pending;
  ComObject<IMGDFReadOnlyVirtualFileSystem> previous;
  EXPECT_TRUE(FAILED(state->BeginSave(pending.Assign(), previous.Assign())));
  EXPECT_FALSE(pending);
  EXPECT_FALSE(previous);
  EXPECT_EQ(1, std::distance(
                   fs::directory_iterator(Resources::Instance().SaveBaseDir()),
                   fs::directory_iterator()));
}

TEST_F(SaveManagerTests, InterruptedReplacementRestoresBackup) {
  auto state = ExistingSave();
  const fs::path root(Resources::Instance().SaveDir("existing"));
  fs::rename(root / "data", root / "backup");
  ComObject<IMGDFReadOnlyVirtualFileSystem> saved;
  ASSERT_EQ(S_OK, state->GetVFS(saved.Assign()));
  EXPECT_EQ("old", Read(root / "data" / "value.txt"));
  EXPECT_FALSE(fs::exists(root / "backup"));
}

TEST_F(SaveManagerTests, FailedReplacementRestoresPreviousData) {
  auto state = ExistingSave();
  ComObject<IMGDFWriteableVirtualFileSystem> pending;
  ASSERT_EQ(S_OK, state->BeginSave(pending.Assign(), nullptr));
  fs::remove_all(PendingPath(pending));
  pending.Clear();
  const fs::path root(Resources::Instance().SaveDir("existing"));
  EXPECT_EQ("old", Read(root / "data" / "value.txt"));
  EXPECT_FALSE(fs::exists(root / "backup"));
}

TEST_F(SaveManagerTests, LockedPreviousDataDoesNotCrashOrReplaceSave) {
  auto state = ExistingSave();
  const fs::path root(Resources::Instance().SaveDir("existing"));
  ComObject<IMGDFWriteableVirtualFileSystem> pending;
  ASSERT_EQ(S_OK, state->BeginSave(pending.Assign(), nullptr));
  const fs::path pendingPath = PendingPath(pending);
  std::ofstream(pendingPath / "value.txt") << "new";
  const HANDLE reader =
      CreateFileW((root / "data" / "value.txt").c_str(), GENERIC_READ,
                  FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, reader);
  pending.Clear();
  CloseHandle(reader);
  EXPECT_EQ("old", Read(root / "data" / "value.txt"));
  EXPECT_EQ("new", Read(pendingPath / "value.txt"));
}
