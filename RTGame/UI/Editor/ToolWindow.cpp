#include "ToolWindow.h"
#include "Render/Device.h"
#include "Render/Resource.h"
#include "Render/RenderManager.h"
#include "Render/Gizmo.h"
#include "Common/Files.h"
#include "Game/GameStage.h"
#include "../DearImGui/imgui.h"

static std::unique_ptr< Resource > LoadIconTexture( CommandList& commandList, const wchar_t* path, int slot )
{
  auto& device = RenderManager::GetInstance().GetDevice();

  auto fileData = ReadFileToMemory( path );
  if ( fileData.empty() )
  {
    assert( false );
    return nullptr;
  }

  return device.Load2DTexture( commandList, std::move( fileData ), slot, path, device.GetDearImGuiHeap() );
}

ToolWindow::ToolWindow( CommandList& commandList )
{
  moveIcon    = LoadIconTexture( commandList, L"Content/Textures/Editor/move.dds",    1 );
  rotateIcon  = LoadIconTexture( commandList, L"Content/Textures/Editor/rotate.dds",  2 );
  scaleIcon   = LoadIconTexture( commandList, L"Content/Textures/Editor/scale.dds",   3 );
  wetnessIcon = LoadIconTexture( commandList, L"Content/Textures/Editor/wetness.dds", 4 );
}

ToolWindow::ActiveTool ToolWindow::GetActiveTool() const
{
  return activeTool;
}

GizmoType ToolWindow::GetActiveGizmo() const
{
  switch ( activeTool )
  {
  case ActiveTool::Move:
    return GizmoType::Move;
  case ActiveTool::Rotate:
    return GizmoType::Rotate;
  case ActiveTool::Scale:
    return GizmoType::Scale;
  }

  return GizmoType::None;
}

void ToolWindow::Tick( CommandList& commandList, double timeElapsed )
{
  auto iconSize    = ImVec2( 32, 32 );
  auto normalColor = ImVec4( 0.50f, 0.50f, 0.50f, 1.00f );
  auto activeColor = ImVec4( 1.00f, 1.00f, 1.00f, 1.00f );

  if ( ImGui::Begin( "Tools", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
  {
    if ( ImGui::ImageButton( moveIcon->GetImTextureId(), iconSize, ImVec2( 0, 0 ), ImVec2( 1, 1 ), -1, ImVec4( 0, 0, 0, 0 ), activeTool == ActiveTool::Move ? activeColor : normalColor ) )
      activeTool = ActiveTool::Move;
    
    ImGui::SameLine();
    if ( ImGui::ImageButton( rotateIcon->GetImTextureId(), iconSize, ImVec2( 0, 0 ), ImVec2( 1, 1 ), -1, ImVec4( 0, 0, 0, 0 ), activeTool == ActiveTool::Rotate ? activeColor : normalColor ) )
      activeTool = ActiveTool::Rotate;
    
    ImGui::SameLine();
    if ( ImGui::ImageButton( scaleIcon->GetImTextureId(), iconSize, ImVec2( 0, 0 ), ImVec2( 1, 1 ), -1, ImVec4( 0, 0, 0, 0 ), activeTool == ActiveTool::Scale ? activeColor : normalColor ) )
      activeTool = ActiveTool::Scale;

    ImGui::SameLine();
    if ( ImGui::ImageButton( wetnessIcon->GetImTextureId(), iconSize, ImVec2( 0, 0 ), ImVec2( 1, 1 ), -1, ImVec4( 0, 0, 0, 0 ), activeTool == ActiveTool::Wetness ? activeColor : normalColor ) )
      activeTool = ActiveTool::Wetness;
  }
  ImGui::End();

  if ( activeTool == ActiveTool::Wetness && gameStage )
  {
    if ( ImGui::Begin( "Wetness tool", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
      auto gswo = gameStage->GetWetnessOrigin();
      auto gsws = gameStage->GetWetnessSize();
      auto gswd = gameStage->GetWetnessDensity();

      if ( ImGui::InputInt2( "Area origin",  &gswo.x )
        || ImGui::InputInt2( "Area size",    &gsws.x )
        || ImGui::InputInt ( "Area density", &gswd   ) )
        gameStage->ChangeWetnessArea( commandList, gswo.x, gswo.y, gsws.x, gsws.y, gswd );

      ImGui::DragFloat( "Brush strength", &wetnessBrushStrength, 0.05f, -1,    1  );
      ImGui::DragFloat( "Brush radius",   &wetnessBrushRadius,   0.05f, 0.01f, 50 );
      ImGui::DragFloat( "Brush exponent", &wetnessBrushExponent, 0.05f, 0.1f,  10 );
    }
    ImGui::End();
  }
}

void ToolWindow::SetGameStage( GameStage* gameStage )
{
  this->gameStage = gameStage;
}

void ToolWindow::GetWetnessParams( float& brushStrength, float& brushRadius, float& brushExponent ) const
{
  brushStrength = wetnessBrushStrength;
  brushRadius   = wetnessBrushRadius;
  brushExponent = wetnessBrushExponent;
}

