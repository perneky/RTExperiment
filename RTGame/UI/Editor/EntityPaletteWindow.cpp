#include "EntityPaletteWindow.h"
#include "Game/GameDefinition.h"
#include "Game/GameStage.h"
#include "../DearImGui/imgui.h"

static bool Filter( const char* a, const char* b )
{
  auto bBase = b;

  while ( *a )
  {
    while ( *a && tolower( *a ) == tolower( *b ) )
    {
      a++;
      b++;

      if ( *b == 0 )
        return true;
    }

    b = bBase;
    
    if ( *a )
      a++;
  }

  return false;
}

EntityPaletteWindow::EntityPaletteWindow()
{
  for ( auto& entity : gameDefinition.GetEntities() )
  {
    entitiesByGUID[ entity.first ] = N( entity.second.name.data() );
    entitiesByName[ N( entity.second.name.data() ) ] = entity.first;
  }
}

EntityPaletteWindow::~EntityPaletteWindow()
{
}

void EntityPaletteWindow::Tick( CommandList& commandList, double timeElapsed )
{
  if ( ImGui::Begin( "Entity palette", nullptr, ImGuiWindowFlags_None ) )
  {
    ImGui::InputText( "Filter", filter, sizeof( filter ) );

    if ( ImGui::BeginChild( "entityList", ImVec2( 0, -25 ), true ) )
    {
      if ( filter[ 0 ] == 0 || Filter( "Point light", filter ) )
        if ( ImGui::Selectable( "Point light", pointLightSelected ) )
        {
          pointLightSelected = true;
          spotLightSelected  = false;
          entitySelected     = {};
        }

      if ( filter[ 0 ] == 0 || Filter( "Spot light", filter ) )
        if ( ImGui::Selectable( "Spot light", spotLightSelected ) )
        {
          pointLightSelected = false;
          spotLightSelected  = true;
          entitySelected     = {};
        }

      auto onEntitySelected = [this]( const GUID& guid )
      {
        pointLightSelected = false;
        spotLightSelected  = false;
        entitySelected     = guid;
      };

      for ( auto& entity : entitiesByName )
        if ( filter[ 0 ] == 0 || Filter( entity.first.data(), filter ) )
          if ( ImGui::Selectable( entity.first.data(), entitySelected == entity.second ) )
            onEntitySelected( entity.second );

    }
    ImGui::EndChild();

    if ( ImGui::Button( "Place selected" ) )
    {
      if ( pointLightSelected )
        SigPlacePointLight();
      else if ( spotLightSelected )
        SigPlaceSpotLight();
      else if ( entitySelected != GUID {} )
        SigPlaceModel( entitySelected );
    }

    ImGui::SameLine();

    if ( ImGui::Button( "Remove selected" ) )
      SigRemoveSelectedEntity();
  }
  ImGui::End();
}
