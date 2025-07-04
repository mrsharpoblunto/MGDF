#include "StdAfx.h"

#include "DisplayTests.hpp"

#include <MGDF/ComObject.hpp>
#include <sstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

DisplayTests::~DisplayTests(void) {}

DisplayTests::DisplayTests() : _testHDR(false) {}

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
          return TestStep::FAILED;
        }
        return TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->Text.AddLine(
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
        state->Text.AddLine(
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
      });
}

}  // namespace Test
}  // namespace MGDF