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
  host->GetSaves(_saves.Assign());

  Step([this](auto host, auto state) {
    state->AddLine("");
    state->AddLine("Load/Save Tests");
    state->AddLine("");

    _saves->RemoveSave(0);
    state->AddLine("Save game state");

    _saves->CreateGameState(_state.Assign());

    if (!_state->IsNew()) {
      return TestStep::FAILED;
    }
    _state->SetMetadata("key", "value");
    _state->SetMetadata("key1", "value1");

    if (FAILED(_saves->AppendSave(_state, _pending.Assign()))) {
      return TestStep::FAILED;
    } else {
      size_t size;
      std::wstring saveDir;
      _pending->GetSaveDataLocation(nullptr, &size);
      saveDir.resize(size);
      _pending->GetSaveDataLocation(saveDir.data(), &size);

      std::wstring saveFile(saveDir);
      saveFile += L"currentState.txt";
      std::ofstream out(saveFile.c_str(), std::ios::out);
      out << 2;
      out.close();
      return TestStep::PASSED;
    }
  })
      .Step([this](auto host, auto state) {
        state->AddLine("Search saved game state");
        // we didn't complete saving yet so it shouldn't appear in the list
        if (_saves->GetSaveCount() != 0) {
          return TestStep::FAILED;
        } else {
          // free the pending save to flush the save to permanent storage.
          // The save should no longer be marked as new as it should now
          // have persisted data
          _pending = nullptr;
          if (_state->IsNew()) {
            return TestStep::FAILED;
          }

          _state = nullptr;
          if (SUCCEEDED(_saves->GetSave(0, _state.Assign())) &&
              _saves->GetSaveCount() == 1) {
            return TestStep::PASSED;
          } else {
            return TestStep::FAILED;
          }
        }
      })
      .Step([this](auto host, auto state) {
        state->AddLine("Load game state");

        // check the version
        MGDF::Version version;
        _state->GetVersion(&version);
        if (version.Major != 0 || version.Minor != 1) {
          return TestStep::FAILED;
        }

        size_t size;

        // check the metadata
        std::string metadataValue;
        _state->GetMetadata("key", nullptr, &size);
        metadataValue.resize(size);
        _state->GetMetadata("key", metadataValue.data(), &size);
        if (metadataValue != "value") {
          return TestStep::FAILED;
        }

        // check the saved data
        std::wstring saveDir;
        _state->GetSaveDataLocation(nullptr, &size);
        saveDir.resize(size);
        _state->GetSaveDataLocation(saveDir.data(), &size);

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
            ComObject<IRenderSettingsManager> settings;
            host->GetRenderSettings(settings.Assign());
            MGDF::FullScreenDesc desc;
            settings->GetFullscreen(&desc);
            desc.FullScreen = !desc.FullScreen;
            settings->SetFullscreen(&desc);
            settings->ApplySettings();
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
}  // namespace Test

}  // namespace Test
}  // namespace MGDF