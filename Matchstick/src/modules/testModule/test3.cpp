#include "StdAfx.h"

#include "Test3.hpp"

#include <fstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

Test3::~Test3(void) {}

Test3::Test3() { _testState = 0; }

TestModule *Test3::NextTestModule() { return NULL; }

void Test3::Update(ISimHost *host, TextManagerState *state) {
  if (host->GetInput()->IsKeyPress(VK_ESCAPE)) {
    host->ShutDown();
  }

  if (_testState == 0) {
    state->AddLine("");
    state->AddLine("Load/Save Tests");
    state->AddLine("");

    host->RemoveSave("testsave");
    state->AddLine("Save game state");

    UINT32 size = 0;
    if (MGDF_ERR_BUFFER_TOO_SMALL != host->BeginSave("testsave", NULL, &size)) {
      state->SetStatus(RED, "[Test Failed]");
      _testState = 999;
    } else {
      wchar_t *saveDir = new wchar_t[size];
      if (host->BeginSave("testsave", saveDir, &size) == MGDF_OK) {
        std::wstring saveFile(saveDir);
        saveFile += L"currentState.txt";
        std::ofstream out(saveFile.c_str(), std::ios::out);
        out << 2;
        out.close();
        state->SetStatus(GREEN, "[Test Passed]");
        state->AddLine("Search saved game state");
        _testState = 1;
      } else {
        state->SetStatus(RED, "[Test Failed]");
        _testState = 999;
      }
      delete[] saveDir;
    }
  } else if (_testState == 1) {
    // we didn't complete saving yet so it shouldn't appear in the list
    if (host->GetSaves()->Size() != 0) {
      state->SetStatus(RED, "[Test Failed]");
      _testState = 999;
    } else {
      if (host->CompleteSave("testsave") == MGDF_OK &&
          host->GetSaves()->Size() == 1 &&
          strcmp(host->GetSaves()->Get(0), "testsave") == 0) {
        state->SetStatus(GREEN, "[Test Passed]");
        state->AddLine("Load game state");

        unsigned size = 0;
        MGDF::Version version;
        if (MGDF_ERR_BUFFER_TOO_SMALL !=
            host->Load("testsave", NULL, &size, version)) {
          state->SetStatus(RED, "[Test Failed]");
          _testState = 999;
        } else {
          wchar_t *saveDir = new wchar_t[size];
          if (host->Load("testsave", saveDir, &size, version) == MGDF_OK) {
            // make sure the version we loaded is the same that we saved.
            if (version.Major != 0 || version.Minor != 1) {
              state->SetStatus(RED, "[Test Failed - Invalid version]");
              _testState = 999;
            } else {
              std::wstring saveFile(saveDir);
              saveFile += L"currentState.txt";
              std::ifstream in(saveFile.c_str(), std::ios::in);
              if (in.fail()) {
                _testState = 3;
              } else {
                in >> _testState;
                in.close();
                if (_testState != 2) _testState = 3;
              }
            }
          } else {
            state->SetStatus(RED, "[Test Failed - Unable to load]");
            _testState = 999;
          }
          delete[] saveDir;
        }
      } else {
        state->SetStatus(RED, "[Test Failed]");
        _testState = 999;
      }
    }
  } else if (_testState == 2) {
    state->SetStatus(GREEN, "[Test Passed]");
    _testState = 4;
  } else if (_testState == 3) {
    state->SetStatus(RED, "[Test Failed]");
    _testState = 999;
  } else if (_testState == 4) {
    state->AddLine("Testing custom VFS archive handler registration");

    bool success = false;
    IFile *file = host->GetVFS()->GetFile(L"test.fakearchive/testfile.txt");
    if (file != NULL) {
      IFileReader *reader = nullptr;
      if (MGDF_OK == file->Open(&reader)) {
        UINT32 size = static_cast<UINT32>(reader->GetSize());
        char *data = new char[size];
        reader->Read(data, size);
        success = strncmp(data, "hello world", size) == 0;
        delete[] data;
        reader->Close();
      }
    }

    if (!success) {
      state->SetStatus(RED, "[Test Failed]");
      _testState = 999;
    } else {
      state->SetStatus(GREEN, "[Test Passed]");
      _testState++;
    }
  } else if (_testState == 5) {
    state->AddLine(
        "Press [ALT]+[ENTER] to toggle fullscreen/windowed mode. Then press "
        "[Y/N] if this works correctly");
    _testState++;
  } else if (_testState == 6 && host->GetInput()->IsKeyPress('Y')) {
    ++_testState;
    state->SetStatus(GREEN, "[Test Passed]");
    state->AddLine(
        "Press [ALT]+[F12] to toggle the  information overlay. Then press "
        "[Y/N] if this works correctly");
  } else if (_testState == 6 && host->GetInput()->IsKeyPress('N')) {
    _testState = 999;
    state->SetStatus(RED, "[Test Failed]");
  } else if (_testState == 7 && host->GetInput()->IsKeyPress('Y')) {
    _testState = 999;
    state->SetStatus(GREEN, "[Test Passed]");
  } else if (_testState == 7 && host->GetInput()->IsKeyPress('N')) {
    _testState = 999;
    state->SetStatus(RED, "[Test Failed]");
  } else if (_testState == 999) {
    _testState++;
    state->AddLine(
        "All tests complete. Press the [ESC] key to exit (then make sure there "
        "were no memory leaks)");
  }
}

}  // namespace Test
}  // namespace MGDF