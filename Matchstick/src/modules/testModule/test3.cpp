#include "StdAfx.h"

#include "Test3.hpp"

#include <MGDF/ComObject.hpp>
#include <fstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

Test3::~Test3(void) {}

Test3::Test3() {}

TestModule *Test3::NextTestModule() { return nullptr; }

void Test3::Setup(ISimHost *host) {
  host->GetInput(_input.Assign());

  Step([](auto host, auto state) {
    state->AddLine("");
    state->AddLine("Load/Save Tests");
    state->AddLine("");

    host->RemoveSave("testsave");
    state->AddLine("Save game state");

    UINT32 size = 0;
    if (MGDF_ERR_BUFFER_TOO_SMALL !=
        host->BeginSave("testsave", nullptr, &size)) {
      return TestStep::FAILED;
    } else {
      std::wstring saveDir;
      saveDir.resize(size - 1);  // exclude null terminator
      if (host->BeginSave("testsave", saveDir.data(), &size) == MGDF_OK) {
        std::wstring saveFile(saveDir);
        saveFile += L"currentState.txt";
        std::ofstream out(saveFile.c_str(), std::ios::out);
        out << 2;
        out.close();
        return TestStep::PASSED;
      } else {
        return TestStep::FAILED;
      }
    }
  })
      .Step([](auto host, auto state) {
        state->AddLine("Search saved game state");
        // we didn't complete saving yet so it shouldn't appear in the list
        if (host->GetSaves()->Size() != 0) {
          return TestStep::FAILED;
        } else {
          if (host->CompleteSave("testsave") == MGDF_OK &&
              host->GetSaves()->Size() == 1 &&
              strcmp(host->GetSaves()->Get(0), "testsave") == 0) {
            return TestStep::PASSED;
          } else {
            return TestStep::FAILED;
          }
        }
      })
      .Step([](auto host, auto state) {
        state->AddLine("Load game state");

        unsigned size = 0;
        MGDF::Version version;
        if (MGDF_ERR_BUFFER_TOO_SMALL !=
            host->Load("testsave", nullptr, &size, version)) {
          return TestStep::FAILED;
        } else {
          std::wstring saveDir;
          saveDir.resize(size - 1);  // exclude null terminator
          if (host->Load("testsave", saveDir.data(), &size, version) ==
              MGDF_OK) {
            // make sure the version we loaded is the same that we saved.
            if (version.Major != 0 || version.Minor != 1) {
              return TestStep::FAILED;
            } else {
              std::wstring saveFile(saveDir);
              saveFile += L"currentState.txt";
              std::ifstream in(saveFile.c_str(), std::ios::in);
              if (in.fail()) {
                return TestStep::FAILED;
              } else {
                int data;
                in >> data;
                in.close();
                return data == 2 ? TestStep::PASSED : TestStep::FAILED;
              }
            }
          } else {
            return TestStep::FAILED;
          }
        }
      })
      .Step([](auto host, auto state) {
        state->AddLine("Testing custom VFS archive handler registration");

        bool success = false;
        ComObject<IVirtualFileSystem> vfs;
        host->GetVFS(vfs.Assign());

        ComObject<IFile> file;
        if (vfs->GetFile(L"test.fakearchive/testfile.txt", file.Assign())) {
          ComObject<IFileReader> reader;
          if (!FAILED(file->Open(reader.Assign()))) {
            UINT32 size = static_cast<UINT32>(reader->GetSize());
            std::string data;
            data.resize(size);
            reader->Read(data.data(), size);
            return strncmp(data.c_str(), "hello world", size) == 0
                       ? TestStep::PASSED
                       : TestStep::FAILED;
          }
        }
        return TestStep::FAILED;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine(
            "Press [F] to toggle fullscreen/windowed mode. Then press "
            "[Y/N] if this works correctly");
      })
      .Step([this](auto host, auto state) {
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return TestStep::FAILED;
        } else {
          if (_input->IsKeyPress('F')) {
            MGDF::FullScreenDesc desc;
            host->GetRenderSettings()->GetFullscreen(&desc);
            desc.FullScreen = !desc.FullScreen;
            host->GetRenderSettings()->SetFullscreen(&desc);
            host->GetRenderSettings()->ApplySettings();
          }
          return TestStep::CONT;
        }
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine(
            "Press [ALT]+[F12] to toggle the  information overlay. Then press "
            "[Y/N] if this works correctly");
      })
      .Step([this](auto host, auto state) {
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return TestStep::FAILED;
        }
        return TestStep::CONT;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine(
            "All tests complete. Press the [ESC] key to exit (then make sure "
            "there "
            "were no memory leaks)");
      });
}

}  // namespace Test
}  // namespace MGDF