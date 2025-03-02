#include "stdafx.h"

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/vfs/MGDFReadOnlyVirtualFileSystemComponent.hpp"
#include "../../src/core/vfs/archive/zip/ZipArchiveHandlerImpl.hpp"
#include "VFSCommon.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;

class ReadOnlyVFSTests : public ::testing::Test {
 protected:
  void SetUp() override {
    HINSTANCE inst;
    inst = (HINSTANCE)GetModuleHandleW(L"core.tests.exe");
    Resources::Instance(inst);
    Resources::Instance().SetUserBaseDir("junkship", true);

    CreateReadOnlyVirtualFileSystemComponent(_vfs);
    _vfs->RegisterArchiveHandler(zip::CreateZipArchiveHandlerImpl());
  }

  void TearDown() override { _vfs.Clear(); }

  ComObject<IReadOnlyVirtualFileSystemComponent> _vfs;
};

typedef ComString<&IMGDFReadOnlyFile::GetLogicalName> GetLogicalName;

// check that zip archives are enumerated correctly by the vfs
TEST_F(ReadOnlyVFSTests, ZipArchiveTests) {
  _vfs->Mount(
      (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
          .c_str());

  ComObject<IMGDFReadOnlyFile> root;
  _vfs->GetRoot(root.Assign());
  EXPECT_EQ(6, root->GetChildCount());

  ComObject<IMGDFReadOnlyFile> file;
  for (auto f : std::vector({L"game.xml", L"gameIcon.png", L"preferences.xml",
                             L"preferenceTemplates.xml"})) {
    ASSERT_TRUE(_vfs->GetFile(f, file.Assign()));
    EXPECT_STREQ(f, GetLogicalName(file).c_str());
  }

  ASSERT_TRUE(_vfs->GetFile(L"boot/gameState.xml", file.Assign()));
  EXPECT_STREQ(L"gameState.xml", GetLogicalName(file).c_str());
  ASSERT_TRUE(_vfs->GetFile(L"boot/persistency.xml", file.Assign()));
  EXPECT_STREQ(L"persistency.xml", GetLogicalName(file).c_str());
  ASSERT_TRUE(_vfs->GetFile(L"content/test.lua", file.Assign()));
  EXPECT_STREQ(L"test.lua", GetLogicalName(file).c_str());
  ASSERT_TRUE(_vfs->GetFile(L"content", file.Assign()));
  EXPECT_TRUE(file->IsFolder());
}

// check that files inside enumerated archives can be read correctly
TEST_F(ReadOnlyVFSTests, ZipArchiveContentTests) {
  _vfs->Mount(
      (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
          .c_str());

  ComObject<IMGDFReadOnlyFile> file;
  EXPECT_TRUE(_vfs->GetFile(L"content/test.lua", file.Assign()));
  std::vector<std::string> list;
  {
    ComObject<IMGDFFileReader> reader;
    EXPECT_EQ(S_OK, file->Open(reader.Assign()));
    ASSERT_TRUE(reader);

    ReadLines(reader, list);
  }

  // see if the file has as many lines as we expect
  EXPECT_EQ(20, list.size());
  // check to see the first and last lines are as expected
  EXPECT_EQ("class 'ConsoleStorageListener'(MGDF.StorageListener)", list[0]);
  EXPECT_EQ("end", list[19]);
}

// check that traversing the VFS archive tree works
TEST_F(ReadOnlyVFSTests, ZipArchiveTraversalTests) {
  _vfs->Mount(
      (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
          .c_str());

  ComObject<IMGDFReadOnlyFile> root;
  _vfs->GetRoot(root.Assign());
  ASSERT_TRUE(root);
  UINT64 childCount = root->GetChildCount();
  EXPECT_EQ(6, childCount);

  EXPECT_EQ(S_OK, root->GetAllChildren(nullptr, &childCount));
  EXPECT_EQ(6, childCount);

  ComArray<IMGDFReadOnlyFile> buffer2(6);

  childCount = 5;
  EXPECT_EQ(E_NOT_SUFFICIENT_BUFFER,
            root->GetAllChildren(buffer2.Data(), &childCount));
  EXPECT_EQ(6, childCount);

  ASSERT_HRESULT_SUCCEEDED(root->GetAllChildren(buffer2.Data(), &childCount));
  ASSERT_EQ(6, childCount);

  EXPECT_STREQ(L"gameIcon.png", GetLogicalName(buffer2[0]).c_str());
  EXPECT_STREQ(L"preferenceTemplates.xml", GetLogicalName(buffer2[1]).c_str());
  EXPECT_STREQ(L"preferences.xml", GetLogicalName(buffer2[2]).c_str());
  EXPECT_STREQ(L"boot", GetLogicalName(buffer2[3]).c_str());
  EXPECT_STREQ(L"game.xml", GetLogicalName(buffer2[4]).c_str());
  EXPECT_STREQ(L"content", GetLogicalName(buffer2[5]).c_str());

  ComObject<IMGDFReadOnlyFile> parent;
  buffer2[0]->GetParent(parent.Assign());
  EXPECT_EQ(root.Get(), parent.Get());
}

// check that the standard file is enumerated correctly by the vfs
TEST_F(ReadOnlyVFSTests, FileSystemTests) {
  _vfs->Mount(
      (Resources::Instance().RootDir() + L"../../../tests/content").c_str());

  ComObject<IMGDFReadOnlyFile> root;
  _vfs->GetRoot(root.Assign());
  ASSERT_EQ(5, root->GetChildCount());

  for (auto f : std::vector({L"test.zip", L"console.json", L"preferences.json",
                             L"gameState.json", L"Update.json"})) {
    ComObject<IMGDFReadOnlyFile> file;
    ASSERT_TRUE(_vfs->GetFile(f, file.Assign()));
    EXPECT_STREQ(f, GetLogicalName(file).c_str());
    ASSERT_TRUE(root->GetChild(f, file.Assign()));
    EXPECT_STREQ(f, GetLogicalName(file).c_str());
  }
}

// check that files in the standard file can be read from the vfs correctly
TEST_F(ReadOnlyVFSTests, FileSystemContentTests) {
  EXPECT_TRUE(_vfs->Mount(
      (Resources::Instance().RootDir() + L"../../../tests/content").c_str()));

  ComObject<IMGDFReadOnlyFile> file;
  ASSERT_TRUE(_vfs->GetFile(L"console.json", file.Assign()));

  std::vector<std::string> list;
  {
    ComObject<IMGDFFileReader> reader;
    ASSERT_HRESULT_SUCCEEDED(file->Open(reader.Assign()));
    ASSERT_TRUE(reader);
    ReadLines(reader, list);
  }

  // see if the file has as many lines as we expect
  EXPECT_EQ(17, list.size());
  // check to see the first and last lines are as expected
  EXPECT_EQ("{", list[0]);
  EXPECT_EQ("}", list[16]);
}