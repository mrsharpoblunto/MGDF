#include "StdAfx.h"

#include "LoadSaveTests.hpp"

#include <MGDF/ComObject.hpp>
#include <sstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

LoadSaveTests::~LoadSaveTests(void) {}

LoadSaveTests::LoadSaveTests() {}

void LoadSaveTests::Setup(IMGDFSimHost *host) {
  host->GetInput(_input.Assign());
  host->GetSaves(_saves.Assign());

  Step([this](auto state) {
    state->Text.AddLine("");
    state->Text.AddLine("Load/Save Tests");
    state->Text.AddLine("");
    while (_saves->GetSaveCount() > 0) {
      ComObject<IMGDFGameState> s;
      _saves->GetSave(0, s.Assign());
      _saves->DeleteSave(s);
    }
    state->Text.AddLine("Save game state");

    _saves->CreateGameState(_state.Assign());

    if (!_state->IsNew()) {
      return state->Fail("Expected a newly created game state to be new");
    }
    _state->SetMetadata("key", "value");
    _state->SetMetadata("key1", "value1");

    if (FAILED(_state->BeginSave(_pending.Assign()))) {
      return state->Fail("BeginSave failed on the new game state");
    } else {
      ComObject<IMGDFWriteableFile> saveFile;
      _pending->GetFile(L"subfolder/currentState.txt", saveFile.Assign());
      if (saveFile->Exists()) {
        return state->Fail(
            "subfolder/currentState.txt should not exist before being "
            "written");
      }

      {
        ComObject<IMGDFFileWriter> saveWriter;
        if (FAILED(saveFile->OpenWrite(saveWriter.Assign()))) {
          return state->Fail(
              "Unable to open subfolder/currentState.txt for writing");
        }
        std::string data("2");
        saveWriter->Write(data.data(), static_cast<UINT32>(data.size()));
      }

      if (!saveFile->Exists() || saveFile->IsFolder()) {
        return state->Fail(
            "subfolder/currentState.txt should exist as a file after being "
            "written");
      }
      return TestStep::PASSED;
    }
  })
      .Step([this](auto state) {
        state->Text.AddLine("Search saved game state");
        // we didn't complete saving yet so it shouldn't appear in the list
        if (_saves->GetSaveCount() != 0) {
          std::ostringstream oss;
          oss << "Expected no saves before the pending save completes, found "
              << _saves->GetSaveCount();
          return state->Fail(oss.str());
        } else {
          // free the pending save to flush the save to permanent storage.
          // The save should no longer be marked as new as it should now
          // have persisted data
          _pending.Clear();
          if (_state->IsNew()) {
            return state->Fail(
                "Game state should no longer be new after the pending save "
                "completed");
          }

          _state.Clear();
          if (SUCCEEDED(_saves->GetSave(0, _state.Assign())) &&
              _saves->GetSaveCount() == 1) {
            return TestStep::PASSED;
          } else {
            std::ostringstream oss;
            oss << "Expected exactly one save to exist, found "
                << _saves->GetSaveCount();
            return state->Fail(oss.str());
          }
        }
      })
      .Step([this](auto state) {
        state->Text.AddLine("Load game state");

        // check the version
        MGDFVersion version;
        _state->GetVersion(&version);
        if (version.Major != 0 || version.Minor != 1) {
          std::ostringstream oss;
          oss << "Expected saved game state version 0.1, got " << version.Major
              << "." << version.Minor;
          return state->Fail(oss.str());
        }

        // check the metadata
        std::string metadataValue =
            ComString<&IMGDFGameState::GetMetadata>(_state, "key");
        if (metadataValue != "value") {
          return state->Fail("Expected metadata 'key' to be 'value', got '" +
                             metadataValue + "'");
        }

        ComObject<IMGDFReadOnlyVirtualFileSystem> vfs;
        if (FAILED(_state->GetVFS(vfs.Assign()))) {
          return state->Fail("Unable to get the VFS for the saved game state");
        }
        ComObject<IMGDFReadOnlyFile> saveFile;
        vfs->GetFile(L"subfolder/currentState.txt", saveFile.Assign());

        ComObject<IMGDFFileReader> saveReader;
        if (FAILED(saveFile->Open(saveReader.Assign()))) {
          return state->Fail(
              "Unable to open subfolder/currentState.txt from the save");
        }

        std::string data;
        data.resize(saveReader->GetSize());
        saveReader->Read(data.data(), static_cast<UINT32>(data.size()));

        return data == "2"
                   ? TestStep::PASSED
                   : state->Fail(
                         "Expected saved file content '2', got '" + data +
                         "'");
      })
      .Step([host](auto state) {
        state->Text.AddLine("Testing custom VFS archive handler registration");

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
                       : state->Fail(
                             "Expected archive file content 'hello world', "
                             "got '" +
                             data + "'");
          }
          return state->Fail(
              "Unable to open test.fakearchive/testfile.txt for reading");
        }
        return state->Fail(
            "test.fakearchive/testfile.txt was not found in the VFS");
      });
}

}  // namespace Test
}  // namespace MGDF