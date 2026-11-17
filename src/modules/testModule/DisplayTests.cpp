#include "StdAfx.h"

#include "DisplayTests.hpp"

#include <MGDF/ComObject.hpp>
#include <sstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

namespace {
std::wstring GameWindowTitle() {
  struct Search {
    DWORD ProcessId;
    HWND Window;
  } search{::GetCurrentProcessId(), nullptr};
  ::EnumWindows(
      [](HWND window, LPARAM param) -> BOOL {
        auto &search = *reinterpret_cast<Search *>(param);
        DWORD processId = 0;
        ::GetWindowThreadProcessId(window, &processId);
        if (processId != search.ProcessId || ::GetWindow(window, GW_OWNER) ||
            !::IsWindowVisible(window)) {
          return TRUE;
        }
        search.Window = window;
        return FALSE;
      },
      reinterpret_cast<LPARAM>(&search));
  if (!search.Window) return L"";
  wchar_t title[256]{};
  ::GetWindowTextW(search.Window, title, ARRAYSIZE(title));
  return title;
}

std::wstring ToWide(const char *utf8) {
  const int length = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
  std::wstring result(length > 0 ? length - 1 : 0, L'\0');
  ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, result.data(), length);
  return result;
}
}  // namespace

DisplayTests::~DisplayTests(void) {}

DisplayTests::DisplayTests() : _testHDR(false), _titleDeadline(0) {}

void DisplayTests::Setup(IMGDFSimHost *host) {
  host->GetInput(_input.Assign());
  host->GetRenderSettings(_settings.Assign());

  StepOnce([this](auto state) {
    state->Text.AddLine("");
    state->Text.AddLine("Press [H] to query HDR mode on the current output");
    state->Text.AddLine("Press [Y/N] if this works correctly");
  })
      .Step([host, this](auto state) {
        if (_input->IsKeyPress('H')) {
          _testHDR = !_testHDR;
          ComObject<IMGDFPendingRenderSettingsChange> pending;
          _settings->CreatePendingSettingsChange(pending.Assign());
          pending->SetHDREnabled(_testHDR);

          MGDFOutputDisplayInfo info;
          _settings->GetCurrentOutputDisplayInfo(&info);
          if (info.SupportsHDR) {
            std::ostringstream oss;
            oss << "HDR Display Info: " << info.MinLuminance << "->"
                << info.MaxLuminance
                << "Nits, SDR level: " << info.SDRWhiteLevel;
            state->Text.AddLine(oss.str());
          } else {
            state->Text.AddLine("Warning: HDR Unsupported");
          }
        } else if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return state->Fail("User reported HDR mode query did not work");
        }
        return TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->Text.AddLine(
            "Press [F] to toggle fullscreen/windowed mode. Then press "
            "[Y/N] if this works correctly");
      })
      .Step([host, this](auto state) {
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return state->Fail(
              "User reported fullscreen/windowed toggle did not work");
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
        state->Text.AddLine(
            "Press [ALT]+[F12] to toggle the  information overlay. Then press "
            "[Y/N] if this works correctly");
      })
      .Step([this](auto state) {
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return state->Fail(
              "User reported the information overlay toggle did not work");
        }
        return TestStep::CONT;
      })
      .StepOnce([host, this](auto state) {
        state->Text.AddLine("Window title can be changed");
        host->SetWindowTitle("MGDF title test \xC3\xA9");
        _titleDeadline = ::GetTickCount64() + 2000;
      })
      .Step([this](auto state) {
        if (GameWindowTitle() == L"MGDF title test \u00E9") {
          return TestStep::PASSED;
        }
        if (::GetTickCount64() > _titleDeadline) {
          return state->Fail("The window title did not change to the title set");
        }
        return TestStep::CONT;
      })
      .StepOnce([host, this](auto state) {
        state->Text.AddLine("Window title returns to the game name");
        host->SetWindowTitle(nullptr);
        _titleDeadline = ::GetTickCount64() + 2000;
      })
      .Step([this, host](auto state) {
        ComObject<IMGDFGame> game;
        host->GetGame(game.Assign());
        if (GameWindowTitle() == ToWide(game->GetName())) {
          return TestStep::PASSED;
        }
        if (::GetTickCount64() > _titleDeadline) {
          return state->Fail("The window title did not return to the game name");
        }
        return TestStep::CONT;
      });
}

}  // namespace Test
}  // namespace MGDF