#include "stdafx.h"

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/vfs/MGDFVirtualFileSystemComponentImpl.hpp"
#include "../../src/core/vfs/archive/zip/ZipArchiveHandlerImpl.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::vfs;

SUITE(VFSTests) {
  struct VFSTestFixture {
    VFSTestFixture() {
      HINSTANCE inst;
      inst = (HINSTANCE)GetModuleHandleW(L"core.tests.exe");
      Resources::Instance(inst);
      Resources::Instance().SetUserBaseDir(true, "junkship");

      _vfs = CreateVirtualFileSystemComponentImpl();
      _vfs->RegisterArchiveHandler(zip::CreateZipArchiveHandlerImpl());
    }

    virtual ~VFSTestFixture() {}

    void ReadLines(ComObject<IFileReader> &reader,
                   std::vector<std::string> &list) {
      UINT32 size = static_cast<UINT32>(reader->GetSize());
      char *data = new char[size];
      char *copy = new char[size + 1];
      reader->Read((void *)data, size);

      size_t index = 0;
      for (size_t i = 0; i < size; ++i) {
        if (data[i] != '\r' && data[i] != '\t') {
          copy[index++] = data[i];
        }
      }
      copy[index] = '\0';
      delete[] data;

      char *context = 0;
      char *ptr = strtok_s(copy, "\n", &context);
      while (ptr) {
        list.push_back(std::string(ptr));
        ptr = strtok_s(0, "\n", &context);
      }
      delete[] copy;
    }

   protected:
    ComObject<IVirtualFileSystemComponent> _vfs;
  };

  /**
  check that zip archives are enumerated correctly by the vfs
  */
  TEST_FIXTURE(VFSTestFixture, ZipArchiveTests) {
    _vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
            .c_str());

    ComObject<IFile> root;
    _vfs->GetRoot(root.Assign());
    CHECK_WS_EQUAL(L"test.zip", root->GetName());
    CHECK_EQUAL(6, root->GetChildCount());

    ComObject<IFile> file;
    for (auto f : std::vector({L"game.xml", L"gameIcon.png", L"preferences.xml",
                               L"preferenceTemplates.xml"})) {
      CHECK(_vfs->GetFile(f, file.Assign()));
      CHECK_WS_EQUAL(f, file->GetName());
    }

    CHECK(_vfs->GetFile(L"boot/gameState.xml", file.Assign()));
    CHECK_WS_EQUAL(L"gameState.xml", file->GetName());
    CHECK(_vfs->GetFile(L"boot/persistency.xml", file.Assign()));
    CHECK_WS_EQUAL(L"persistency.xml", file->GetName());
    CHECK(_vfs->GetFile(L"content/test.lua", file.Assign()));
    CHECK_WS_EQUAL(L"test.lua", file->GetName());
    CHECK(_vfs->GetFile(L"content", file.Assign()));
    CHECK(file->IsFolder());
  }

  /*
  check that files inside enumerated archives can be read correctly
  */
  TEST_FIXTURE(VFSTestFixture, ZipArchiveContentTests) {
    _vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
            .c_str());

    ComObject<IFile> file;
    CHECK(_vfs->GetFile(L"content/test.lua", file.Assign()));
    std::vector<std::string> list;
    {
      ComObject<IFileReader> reader;
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
  check that vfs filters and aliases work as expected
  */
  TEST_FIXTURE(VFSTestFixture, AliasAndFilterTests) {
    _vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content/test.zip")
            .c_str());

    ComObject<IFile> root;
    _vfs->GetRoot(root.Assign());
    CHECK(root);
    CHECK_EQUAL(6, root->GetChildCount());

    ComArray<IFile> buffer2(6);
    root->GetAllChildren(buffer2.Data());
    CHECK_WS_EQUAL(L"boot", buffer2[0]->GetName());
    CHECK_WS_EQUAL(L"content", buffer2[1]->GetName());
    CHECK_WS_EQUAL(L"game.xml", buffer2[2]->GetName());
    CHECK_WS_EQUAL(L"gameIcon.png", buffer2[3]->GetName());
    CHECK_WS_EQUAL(L"preferenceTemplates.xml", buffer2[4]->GetName());
    CHECK_WS_EQUAL(L"preferences.xml", buffer2[5]->GetName());
  }

  /**
  check that the standard file is enumerated correctly by the vfs
  */
  TEST_FIXTURE(VFSTestFixture, FileSystemTests) {
    _vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content").c_str());

    ComObject<IFile> root;
    _vfs->GetRoot(root.Assign());
    CHECK_EQUAL(5, root->GetChildCount());

    for (auto f :
         std::vector({L"test.zip", L"console.json", L"preferences.json",
                      L"gameState.json", L"Update.json"})) {
      ComObject<IFile> file;
      CHECK(_vfs->GetFile(f, file.Assign()));
      CHECK_WS_EQUAL(f, file->GetName());
    }
  }

  /**
  check that files in the standard file can be read from the vfs correctly
  */
  TEST_FIXTURE(VFSTestFixture, FileSystemContentTests) {
    CHECK(_vfs->Mount(
        (Resources::Instance().RootDir() + L"../../../tests/content").c_str()));

    ComObject<IFile> file;
    CHECK(_vfs->GetFile(L"console.json", file.Assign()));

    std::vector<std::string> list;
    {
      ComObject<IFileReader> reader;
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