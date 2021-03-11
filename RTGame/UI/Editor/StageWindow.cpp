#include "StageWindow.h"
#include "Game/GameDefinition.h"
#include "Game/GameStage.h"
#include "../DearImGui/imgui.h"

struct StageWindow::Sky
{
  Sky() = default;
  Sky( const GameDefinition::Sky& sky ) 
  {
    guid = sky.guid;
    name = N( sky.name.data() );
  };

  std::string name;
  GUID        guid;
};

StageWindow::StageWindow()
{
  for ( auto& sky : gameDefinition.GetSkies() )
    skies.emplace_back( sky.second );
}

StageWindow::~StageWindow()
{
}

void StageWindow::Tick( CommandList& commandList, double timeElapsed )
{
  if ( !stage )
    return;

  if ( ImGui::Begin( "Stage parameters", nullptr, ImGuiWindowFlags_None ) )
  {
    ImGui::SliderFloat( "Scene exposure", &stage->exposure, 0, 5 );
    ImGui::SliderFloat( "Bloom strength", &stage->bloomStrength, 0, 2 );
    ImGui::SliderFloat( "Bloom threshold", &stage->bloomThreshold, 0, 8 );

    if ( ImGui::BeginCombo( "Sky", selectedSky < 0 ? nullptr : skies[ selectedSky ].name.data(), ImGuiComboFlags_PopupAlignLeft | ImGuiComboFlags_HeightRegular ) )
    {
      int cnt = 0;
      for ( auto& sky : skies )
      {        
        if ( ImGui::Selectable( sky.name.data(), cnt == selectedSky ) )
        {
          selectedSky = cnt;
          stage->SetSky( commandList, sky.guid );
        }
        ++cnt;
      }
      ImGui::EndCombo();
    }
  }
  ImGui::End();
}

void StageWindow::SetStage( GameStage* stage )
{
  this->stage = stage;

  if ( stage )
  {
    auto iter = std::find_if( skies.begin(), skies.end(), [&]( const Sky& sky ) { return sky.guid == stage->skyGUID; } );
    if ( iter != skies.end() )
      selectedSky = int( std::distance( skies.begin(), iter ) );
    else
      selectedSky = -1;
  }
  else
  {
    selectedSky = -1;
  }
}
