#include "stdafx.h"

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/common/MGDFStringImpl.hpp"
#include "../../src/core/vfs/MGDFReadOnlyVirtualFileSystemComponentImpl.hpp"
#include "../../src/core/vfs/archive/zip/ZipArchiveHandlerImpl.hpp"
#include "VFSCommon.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;

SUITE(ReadOnlyVFSTests) {
  struct ReadOnlyVFSTestFixture {
    ReadOnlyVFSTestFixture() {
      HINSTANCE inst;
      inst = (HINSTANCE)GetModuleHandleW(L"core.tests.exe");
      Resources::Instance(inst);
      Resources::Instance().SetUserBaseDir(true, "junkship");

      CreateReadOnlyVirtualFileSystemComponentImpl(_vfs);
      _vfs->RegisterArchiveHandler(zip::CreateZipArchiveHandlerImpl());
    }

    virtual ~ReadOnlyVFSTestFixture() {}

   protected:
    ComObject<IReadOnlyVirtualFileSystemComponent> _vfs;
  };

  typedef StringReader<&IMGDFReadOnlyFile::GetLogicalName> GetLogicalName;

  /**
  check that zip archives are enumerated correctly by the vfs
  */
  TEST_FIXTURE(ReadOnlyVFSTestFixture, ZipArchiveTests) {
    _vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
            .c_str());

    ComObject<IMGDFReadOnlyFile> root;
    _vfs->GetRoot(root.Assign());
    CHECK_WS_EQUAL(L"test.zip", GetLogicalName::Read(root));
    CHECK_EQUAL(6, root->GetChildCount());

    ComObject<IMGDFReadOnlyFile> file;
    for (auto f : std::vector({L"game.xml", L"gameIcon.png", L"preferences.xml",
                               L"preferenceTemplates.xml"})) {
      CHECK(_vfs->GetFile(f, file.Assign()));
      CHECK_WS_EQUAL(f, GetLogicalName::Read(file));
    }

    CHECK(_vfs->GetFile(L"boot/gameState.xml", file.Assign()));
    CHECK_WS_EQUAL(L"gameState.xml", GetLogicalName::Read(file));
    CHECK(_vfs->GetFile(L"boot/persistency.xml", file.Assign()));
    CHECK_WS_EQUAL(L"persistency.xml", GetLogicalName::Read(file));
    CHECK(_vfs->GetFile(L"content/test.lua", file.Assign()));
    CHECK_WS_EQUAL(L"test.lua", GetLogicalName::Read(file));
    CHECK(_vfs->GetFile(L"content", file.Assign()));
    CHECK(file->IsFolder());
  }

  /*
  check that files inside enumerated archives can be read correctly
  */
  TEST_FIXTURE(ReadOnlyVFSTestFixture, ZipArchiveContentTests) {
    _vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
            .c_str());

    ComObject<IMGDFReadOnlyFile> file;
    CHECK(_vfs->GetFile(L"content/test.lua", file.Assign()));
    std::vector<std::string> list;
    {
      ComObject<IMGDFFileReader> reader;
      CHECK_EQUAL(S_OK, file->Open(reader.Assign()));
      CHECK(reader);

      ReadLines(reader, list);
    }

    // see if the file has as many lines as we expect
    CHECK_EQUAL(20, list.size());
    // check to see the first and last lines are as expected
    CHECK_EQUAL("class 'ConsoleStorageListener'(MGDF.StorageListener)",
                list[0]);
    CHECK_EQUAL("end", list[19]);
  }

  /**
  check that traversing the VFS archive tree works
  */
  TEST_FIXTURE(ReadOnlyVFSTestFixture, ZipArchiveTraversalTests) {
    _vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
            .c_str());

    ComObject<IMGDFReadOnlyFile> root;
    _vfs->GetRoot(root.Assign());
    CHECK(root);
    CHECK_EQUAL(6, root->GetChildCount());

    ComArray<IMGDFReadOnlyFile> buffer2(6);
    root->GetAllChildren(buffer2.Data());
    CHECK_WS_EQUAL(L"boot", GetLogicalName::Read(buffer2[0]));
    CHECK_WS_EQUAL(L"content", GetLogicalName::Read(buffer2[1]));
    CHECK_WS_EQUAL(L"game.xml", GetLogicalName::Read(buffer2[2]));
    CHECK_WS_EQUAL(L"gameIcon.png", GetLogicalName::Read(buffer2[3]));
    CHECK_WS_EQUAL(L"preferenceTemplates.xml",
                   GetLogicalName::Read(buffer2[4]));
    CHECK_WS_EQUAL(L"preferences.xml", GetLogicalName::Read(buffer2[5]));

    ComObject<IMGDFReadOnlyFile> parent;
    buffer2[0]->GetParent(parent.Assign());
    CHECK_EQUAL(root, parent);
  }

  /**
  check that the standard file is enumerated correctly by the vfs
  */
  TEST_FIXTURE(ReadOnlyVFSTestFixture, FileSystemTests) {
    _vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content").c_str());

    ComObject<IMGDFReadOnlyFile> root;
    _vfs->GetRoot(root.Assign());
    CHECK_EQUAL(5, root->GetChildCount());

    for (auto f :
         std::vector({L"test.zip", L"console.json", L"preferences.json",
                      L"gameState.json", L"Update.json"})) {
      ComObject<IMGDFReadOnlyFile> file;
      CHECK(_vfs->GetFile(f, file.Assign()));
      CHECK_WS_EQUAL(f, GetLogicalName::Read(file));
    }
  }

  /**
  check that files in the standard file can be read from the vfs correctly
  */
  TEST_FIXTURE(ReadOnlyVFSTestFixture, FileSystemContentTests) {
    CHECK(_vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content").c_str()));

    ComObject<IMGDFReadOnlyFile> file;
    CHECK(_vfs->GetFile(L"console.json", file.Assign()));

    std::vector<std::string> list;
    {
      ComObject<IMGDFFileReader> reader;
      CHECK_EQUAL(S_OK, file->Open(reader.Assign()));
      CHECK(reader);
      ReadLines(reader, list);
    }

    // see if the file has as many lines as we expect
    CHECK_EQUAL(16, list.size());
    // check to see the first and last lines are as expected
    CHECK_EQUAL("{", list[0]);
    CHECK_EQUAL("}", list[15]);
  }
}