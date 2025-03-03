#include "stdafx.h"

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/vfs/MGDFWriteableVirtualFileSystem.hpp"
#include "VFSCommon.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;

class WriteableVFSTests : public ::testing::Test {
 protected:
  void SetUp() override {
    HINSTANCE inst;
    inst = (HINSTANCE)GetModuleHandleW(L"core.tests.exe");
    Resources::Instance(inst);
    Resources::Instance().SetUserBaseDir("junkship", true);

    _vfs = MakeCom<WriteableVirtualFileSystem>(
        (Resources::Instance().RootDir() + L"../../../tests/content").c_str());
  }

  void TearDown() override { _vfs.Clear(); }

  ComObject<WriteableVirtualFileSystem> _vfs;
};

// check that files are enumerated correctly by the vfs
TEST_F(WriteableVFSTests, FileSystemTests) {
  ComObject<IMGDFWriteableFile> root;
  _vfs->GetRoot(root.Assign());
  EXPECT_EQ(5, root->GetChildCount());

  for (auto f : std::vector({L"test.zip", L"console.json", L"preferences.json",
                             L"gameState.json", L"Update.json"})) {
    ComObject<IMGDFWriteableFile> file;
    ASSERT_TRUE(_vfs->GetFile(f, file.Assign()));
    EXPECT_STREQ(f,
                 ComString<&IMGDFWriteableFile::GetPhysicalName>(file).c_str());
  }
}

// check that files are enumerated correctly by the vfs
TEST_F(WriteableVFSTests, WriteTests) {
  ComObject<IMGDFWriteableFile> newFolder;
  _vfs->GetFile(L"newFolder", newFolder.Assign());
  ASSERT_TRUE(newFolder);
  if (newFolder->Exists()) {
    ASSERT_HRESULT_SUCCEEDED(newFolder->Delete());
  }

  // create a nested folder
  ComObject<IMGDFWriteableFile> nestedFolder;
  _vfs->GetFile(L"newFolder/nested", nestedFolder.Assign());
  ASSERT_TRUE(nestedFolder);
  EXPECT_TRUE(!nestedFolder->Exists());
  ASSERT_HRESULT_SUCCEEDED(nestedFolder->CreateFolder());
  EXPECT_TRUE(nestedFolder->Exists());
  EXPECT_TRUE(nestedFolder->IsFolder());

  // fetch a non-existant file
  ComObject<IMGDFWriteableFile> newFile;
  ASSERT_TRUE(nestedFolder->GetChild(L"newFile.txt", newFile.Assign()));
  EXPECT_TRUE(!newFile->Exists());

  // then write to it
  {
    ComObject<IMGDFFileWriter> writer;
    ASSERT_HRESULT_SUCCEEDED(newFile->OpenWrite(writer.Assign()));
    std::string content("some test content");
    writer->Write(content.data(), static_cast<UINT32>(content.length()));
  }
  EXPECT_TRUE(newFile->Exists());

  // then read from it
  {
    ComObject<IMGDFFileReader> reader;
    ASSERT_HRESULT_SUCCEEDED(newFile->Open(reader.Assign()));
    std::vector<std::string> lines;
    ReadLines(reader, lines);
    EXPECT_EQ(1, lines.size());
    EXPECT_EQ("some test content", lines[0]);
  }

  // and then delete it
  ASSERT_HRESULT_SUCCEEDED(newFile->Delete());
  EXPECT_TRUE(!newFile->Exists());

  if (newFolder->Exists()) {
    ASSERT_HRESULT_SUCCEEDED(newFolder->Delete());
  }
}

// check that files can be read from the vfs correctly
TEST_F(WriteableVFSTests, FileSystemContentTests) {
  ComObject<IMGDFWriteableFile> file;
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