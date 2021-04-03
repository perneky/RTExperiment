#include "DebugWindow.h"
#include "../DearImGui/imgui.h"
#include "Render/ShaderStructuresNative.h"

static constexpr int fpsPeriod = 10;

static const char* GetFrameDebugModeName( FrameDebugModeCB mode )
{
  switch ( mode )
  {
  case FrameDebugModeCB::None:
    return "None";
  case FrameDebugModeCB::ShowGI:
    return "GI";
  case FrameDebugModeCB::ShowAO:
    return "AO";
  case FrameDebugModeCB::ShowGIAO:
    return "GI*AO";
  case FrameDebugModeCB::ShowSurfaceNormal:
    return "Surface normal";
  case FrameDebugModeCB::ShowGeometryNormal:
    return "Geometry normal";
  case FrameDebugModeCB::ShowRoughness:
    return "Roughness";
  case FrameDebugModeCB::ShowMetallic:
    return "Metallic";
  case FrameDebugModeCB::ShowBaseReflection:
    return "Base reflection";
  case FrameDebugModeCB::ShowProcessedReflection:
    return "Processed reflection";
  default:
    assert( false );
    return " ";
  }
}

DebugWindow::DebugWindow()
{
  frameDebugMode = FrameDebugModeCB::None;
}

DebugWindow::~DebugWindow()
{
}

void DebugWindow::Tick( CommandList& commandList, double timeElapsed )
{
  fpsAccum += 1.0 / timeElapsed;
  if ( frameCounter++ % fpsPeriod == 0 )
  {
    wchar_t caption[ 100 ];
    framesPerSecond = int( fpsAccum / fpsPeriod );
    swprintf_s( caption, L"RTGame: %3d FPS", framesPerSecond );
    fpsAccum = 0;
  }

  if ( ImGui::Begin( "Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    ImGui::Text( "Frames per second: %d", framesPerSecond );

    ImGui::Checkbox( "Show GI probes", &showGIProbes );

    if ( ImGui::BeginCombo( "Frame debug mode", GetFrameDebugModeName( frameDebugMode ), ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightRegular ) )
    {
#define FDM( m ) if ( ImGui::Selectable( GetFrameDebugModeName( FrameDebugModeCB::m ), frameDebugMode == FrameDebugModeCB::m ) ) frameDebugMode = FrameDebugModeCB::m;
      FDM( None );
      FDM( ShowGI );
      FDM( ShowAO );
      FDM( ShowGIAO );
      FDM( ShowSurfaceNormal );
      FDM( ShowGeometryNormal );
      FDM( ShowRoughness );
      FDM( ShowMetallic );
      FDM( ShowBaseReflection );
      FDM( ShowProcessedReflection );
#undef FDM
      ImGui::EndCombo();
    }
  }
  ImGui::End();
}

bool DebugWindow::GetShowGIProbes() const
{
  return showGIProbes;
}

FrameDebugModeCB DebugWindow::GetFrameDebugMode() const
{
  return frameDebugMode;
}
