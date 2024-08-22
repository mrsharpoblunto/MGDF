#include "StdAfx.h"

#include "Test3.hpp"

#include <MGDF/ComObject.hpp>
#include <sstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

Test3::~Test3(void) {}

Test3::Test3() {}

TestModule *Test3::NextTestModule() { return nullptr; }

void Test3::Setup(IMGDFSimHost *host) {
  host->GetInput(_input.Assign());
  host->GetSaves(_saves.Assign());

  Step([this](auto state) {
    state->AddLine("");
    state->AddLine("Load/Save Tests");
    state->AddLine("");

    while (_saves->GetSaveCount() > 0) {
      ComObject<IMGDFGameState> s;
      _saves->GetSave(0, s.Assign());
      _saves->DeleteSave(s);
    }
    state->AddLine("Save game state");

    _saves->CreateGameState(_state.Assign());

    if (!_state->IsNew()) {
      return TestStep::FAILED;
    }
    _state->SetMetadata("key", "value");
    _state->SetMetadata("key1", "value1");

    if (FAILED(_state->BeginSave(_pending.Assign()))) {
      return TestStep::FAILED;
    } else {
      ComObject<IMGDFWriteableFile> saveFile;
      _pending->GetFile(L"subfolder/currentState.txt", saveFile.Assign());
      if (saveFile->Exists()) {
        return TestStep::FAILED;
      }

      {
        ComObject<IMGDFFileWriter> saveWriter;
        if (FAILED(saveFile->OpenWrite(saveWriter.Assign()))) {
          return TestStep::FAILED;
        }
        std::string data("2");
        saveWriter->Write(data.data(), static_cast<UINT32>(data.size()));
      }

      if (!saveFile->Exists() || saveFile->IsFolder()) {
        return TestStep::FAILED;
      }
      return TestStep::PASSED;
    }
  })
      .Step([this](auto state) {
        state->AddLine("Search saved game state");
        // we didn't complete saving yet so it shouldn't appear in the list
        if (_saves->GetSaveCount() != 0) {
          return TestStep::FAILED;
        } else {
          // free the pending save to flush the save to permanent storage.
          // The save should no longer be marked as new as it should now
          // have persisted data
          _pending.Clear();
          if (_state->IsNew()) {
            return TestStep::FAILED;
          }

          _state.Clear();
          if (SUCCEEDED(_saves->GetSave(0, _state.Assign())) &&
              _saves->GetSaveCount() == 1) {
            return TestStep::PASSED;
          } else {
            return TestStep::FAILED;
          }
        }
      })
      .Step([this](auto state) {
        state->AddLine("Load game state");

        // check the version
        MGDFVersion version;
        _state->GetVersion(&version);
        if (version.Major != 0 || version.Minor != 1) {
          return TestStep::FAILED;
        }

        // check the metadata
        std::string metadataValue =
            ComString<&IMGDFGameState::GetMetadata>(_state, "key");
        if (metadataValue != "value") {
          return TestStep::FAILED;
        }

        ComObject<IMGDFReadOnlyVirtualFileSystem> vfs;
        if (FAILED(_state->GetVFS(vfs.Assign()))) {
          return TestStep::FAILED;
        }
        ComObject<IMGDFReadOnlyFile> saveFile;
        vfs->GetFile(L"subfolder/currentState.txt", saveFile.Assign());

        ComObject<IMGDFFileReader> saveReader;
        if (FAILED(saveFile->Open(saveReader.Assign()))) {
          return TestStep::FAILED;
        }

        std::string data;
        data.resize(saveReader->GetSize());
        saveReader->Read(data.data(), static_cast<UINT32>(data.size()));

        return data == "2" ? TestStep::PASSED : TestStep::FAILED;
      })
      .Step([host](auto state) {
        state->AddLine("Testing custom VFS archive handler registration");

        ComObject<IMGDFReadOnlyVirtualFileSystem> vfs;
        host->GetVFS(vfs.Assign());

        ComObject<IMGDFReadOnlyFile> file;
        if (vfs->GetFile(L"test.fakearchive/testfile.txt", file.Assign())) {
          ComObject<IMGDFFileReader> reader;
          if (!FAILED(file->Open(reader.Assign()))) {
            const UINT32 size = static_cast<UINT32>(reader->GetSize());
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
      .StepOnce([](auto state) {
        state->AddLine(
            "Press [F] to toggle fullscreen/windowed mode. Then press "
            "[Y/N] if this works correctly");
      })
      .Step([host, this](auto state) {
        std::ignore = state;
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return TestStep::FAILED;
        } else {
          if (_input->IsKeyPress('F')) {
            ComObject<IMGDFRenderSettingsManager> settings;
            host->GetRenderSettings(settings.Assign());
            MGDFFullScreenDesc desc;
            settings->GetFullscreen(&desc);
            desc.FullScreen = !desc.FullScreen;

            ComObject<IMGDFPendingRenderSettingsChange> change;
            settings->CreatePendingSettingsChange(change.Assign());
            change->SetFullscreen(&desc);
          }
          return TestStep::CONT;
        }
      })
      .StepOnce([](auto state) {
        state->AddLine(
            "Press [ALT]+[F12] to toggle the  information overlay. Then press "
            "[Y/N] if this works correctly");
      })
      .Step([this](auto state) {
        std::ignore = state;
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return TestStep::FAILED;
        }
        return TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->AddLine(
            "All tests complete. Press the [ESC] key to exit (then make sure "
            "there "
            "were no memory leaks)");
      });
}  // namespace Test

}  // namespace Test
}  // namespace MGDF