#include "stdafx.h"

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/vfs/MGDFWriteableVirtualFileSystem.hpp"
#include "VFSCommon.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;

SUITE(WriteableVFSTests) {
  struct WriteableVFSTestFixture {
    WriteableVFSTestFixture() {
      HINSTANCE inst;
      inst = (HINSTANCE)GetModuleHandleW(L"core.tests.exe");
      Resources::Instance(inst);
      Resources::Instance().SetUserBaseDir("junkship", true);

      _vfs = MakeCom<WriteableVirtualFileSystem>(
          (Resources::Instance().RootDir() + L"../../../tests/content")
              .c_str());
    }

    virtual ~WriteableVFSTestFixture() {}

   protected:
    ComObject<WriteableVirtualFileSystem> _vfs;
  };

  /**
  check that files are enumerated correctly by the vfs
  */
  TEST_FIXTURE(WriteableVFSTestFixture, FileSystemTests) {
    ComObject<IMGDFWriteableFile> root;
    _vfs->GetRoot(root.Assign());
    CHECK_EQUAL(5, root->GetChildCount());

    for (auto f :
         std::vector({L"test.zip", L"console.json", L"preferences.json",
                      L"gameState.json", L"Update.json"})) {
      ComObject<IMGDFWriteableFile> file;
      CHECK(_vfs->GetFile(f, file.Assign()));
      CHECK_WS_EQUAL(f, ComString<&IMGDFWriteableFile::GetPhysicalName>(file));
    }
  }

  /**
  check that files are enumerated correctly by the vfs
  */
  TEST_FIXTURE(WriteableVFSTestFixture, WriteTests) {
    ComObject<IMGDFWriteableFile> newFolder;
    _vfs->GetFile(L"newFolder", newFolder.Assign());
    CHECK(newFolder);
    if (newFolder->Exists()) {
      CHECK_EQUAL(S_OK, newFolder->Delete());
    }

    // create a nested folder
    ComObject<IMGDFWriteableFile> nestedFolder;
    _vfs->GetFile(L"newFolder/nested", nestedFolder.Assign());
    CHECK(nestedFolder);
    CHECK(!nestedFolder->Exists());
    CHECK_EQUAL(S_OK, nestedFolder->CreateFolder());
    CHECK(nestedFolder->Exists());
    CHECK(nestedFolder->IsFolder());

    // fetch a non-existant file
    ComObject<IMGDFWriteableFile> newFile;
    CHECK(nestedFolder->GetChild(L"newFile.txt", newFile.Assign()));
    CHECK(!newFile->Exists());

    // then write to it
    {
      ComObject<IMGDFFileWriter> writer;
      CHECK_EQUAL(S_OK, newFile->OpenWrite(writer.Assign()));
      std::string content("some test content");
      writer->Write(content.data(), static_cast<UINT32>(content.length()));
    }
    CHECK(newFile->Exists());

    // then read from it
    {
      ComObject<IMGDFFileReader> reader;
      CHECK_EQUAL(S_OK, newFile->Open(reader.Assign()));
      std::vector<std::string> lines;
      ReadLines(reader, lines);
      CHECK_EQUAL(1, lines.size());
      CHECK_EQUAL("some test content", lines[0]);
    }

    // and then delete it
    CHECK_EQUAL(S_OK, newFile->Delete());
    CHECK(!newFile->Exists());

    if (newFolder->Exists()) {
      CHECK_EQUAL(S_OK, newFolder->Delete());
    }
  }

  /**
  check that files can be read from the vfs correctly
  */
  TEST_FIXTURE(WriteableVFSTestFixture, FileSystemContentTests) {
    ComObject<IMGDFWriteableFile> file;
    CHECK(_vfs->GetFile(L"console.json", file.Assign()));

    std::vector<std::string> list;
    {
      ComObject<IMGDFFileReader> reader;
      CHECK_EQUAL(S_OK, file->Open(reader.Assign()));
      CHECK(reader);
      ReadLines(reader, list);
    }

    // see if the file has as many lines as we expect
    CHECK_EQUAL(17, list.size());
    // check to see the first and last lines are as expected
    CHECK_EQUAL("{", list[0]);
    CHECK_EQUAL("}", list[16]);
  }
}