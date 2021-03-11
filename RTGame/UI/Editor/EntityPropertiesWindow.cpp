#include "EntityPropertiesWindow.h"
#include "Game/GameDefinition.h"
#include "Game/Entity.h"
#include "Game/ModelEntity.h"
#include "Game/CameraEntity.h"
#include "Game/LightEntity.h"
#include "../DearImGui/imgui.h"

EntityPropertiesWindow::EntityPropertiesWindow()
{
}

EntityPropertiesWindow::~EntityPropertiesWindow()
{
}

void EntityPropertiesWindow::Tick( CommandList& commandList, double timeElapsed )
{
  if ( ImGui::Begin( "Entity parameters", nullptr, ImGuiWindowFlags_None ) )
  {
    if ( id != EntityId() )
    {
      auto entity   = id.GetEntity();
      auto position = entity->GetPosition();
      auto rotation = entity->GetRotation();
      auto scale    = entity->GetScale();

      if ( ImGui::Button( "Duplicate" ) )
        SigDuplicateEntity( id );
      
      ImGui::SameLine();

      if ( ImGui::Button( "Make array..." ) )
      {
        arrayPosOffset = { 0, 0, 0 };
        arrayRotOffset = { 0, 0, 0 };
        arraySizOffset = { 0, 0, 0 };
        arrayCount     = 1;
        makeArray      = true;
      }

      ImGui::Separator();

      float position3[] = { XMVectorGetX( position ), XMVectorGetY( position ), XMVectorGetZ( position ) };
      if ( ImGui::InputFloat3( "Position", position3 ) && !skipChange )
        entity->SetPosition( XMLoadFloat3( (XMFLOAT3*)position3 ) );

      float rotation3[] = { XMVectorGetX( rotation ), XMVectorGetY( rotation ), XMVectorGetZ( rotation ) };
      if ( ImGui::InputFloat3( "Rotation", rotation3 ) && !skipChange )
        entity->SetRotation( XMLoadFloat3( (XMFLOAT3*)rotation3 ) );

      float scale3[] = { XMVectorGetX( scale ), XMVectorGetY( scale ), XMVectorGetZ( scale ) };
      if ( ImGui::InputFloat3( "Scale", scale3 ) && !skipChange )
        entity->SetScale( XMLoadFloat3( (XMFLOAT3*)scale3 ) );

      ImGui::Separator();

      if ( auto e = dynamic_cast< ModelEntity* >( entity ) )
      {
        auto name = N( gameDefinition.GetEntityName( e->GetGUID() ).data() );
        auto aabb = e->CalcAABB();

        float aabbSize[ 3 ] = { aabb.Extents.x * 2, aabb.Extents.y * 2, aabb.Extents.z * 2 };

        ImGui::Text( "ModelEntity" );
        ImGui::InputText( "Name", name.data(), name.size(), ImGuiInputTextFlags_ReadOnly );
        ImGui::InputFloat3( "AABB size", aabbSize, "%.3f", ImGuiInputTextFlags_ReadOnly );

        auto animNames = e->GetAnimationNames();
        if ( !animNames.empty() )
        {
          if ( ImGui::BeginCombo( "Animations", N( e->GetCurrentAnimationName() ).data() ) )
          {
            for ( auto name : animNames )
              if ( ImGui::Selectable( N( name ).data() ) && !skipChange )
                e->SetCurrentAnimationName( name );
            ImGui::EndCombo();
          }
        }

        HandleSlotEntity();
      }
      else if ( auto e = dynamic_cast< LightEntity* >( entity ) )
      {
        ImGui::Text( "LightEntity" );

        if ( ImGui::BeginCombo( "Type", e->GetType() == LightTypeCB::Point ? "Point" : "Spot" ) )
        {
          if ( ImGui::Selectable( "Point" ) && !skipChange )
            e->SetType( LightTypeCB::Point );
          if ( ImGui::Selectable( "Spot" ) && !skipChange )
            e->SetType( LightTypeCB::Spot );

          ImGui::EndCombo();
        }

        bool castShadow = e->GetCastShadow();
        bool foo = castShadow;
        if ( ImGui::Checkbox( "Cast shadow", &castShadow ) && !skipChange )
          e->SetCastShadow( castShadow );

        float color[ 3 ] = { e->GetColor().r, e->GetColor().g, e->GetColor().b };
        if ( ImGui::ColorPicker3( "Color", color ) && !skipChange )
          e->SetColor( Color( color[ 0 ], color[ 1 ], color[ 2 ] ) );

        float v = e->GetIntensity();
        if ( ImGui::SliderFloat( "Intensity", &v, 0, 10 ) && !skipChange )
          e->SetIntensity( v );

        v = e->GetSourceRadius();
        if ( ImGui::SliderFloat( "Source radius", &v, 0, 1 ) && !skipChange )
          e->SetSourceRadius( v );

        v = e->GetAttenuationStart();
        if ( ImGui::SliderFloat( "Attenuation start", &v, 0, e->GetAttenuationEnd() - 0.001f ) && !skipChange )
          e->SetAttenuationStart( v );

        v = e->GetAttenuationEnd();
        if ( ImGui::SliderFloat( "Attenuation end", &v, e->GetAttenuationStart() + 0.001f, 100 ) && !skipChange )
          e->SetAttenuationEnd( v );

        v = e->GetAttenuationFalloff();
        if ( ImGui::SliderFloat( "Attenuation falloff", &v, 0.1f, 50 ) && !skipChange )
          e->SetAttenuationFalloff( v );

        if ( e->GetType() == LightTypeCB::Spot )
        {
          v = e->GetTheta();
          if ( ImGui::SliderFloat( "Inner angle", &v, 0, e->GetPhi() - 1 ) && !skipChange )
            e->SetTheta( v );

          v = e->GetPhi();
          if ( ImGui::SliderFloat( "Outer angle", &v, e->GetTheta() + 1, 160 ) && !skipChange )
            e->SetPhi( v );

          v = e->GetConeFalloff();
          if ( ImGui::SliderFloat( "Cone falloff", &v, 0.1f, 50 ) && !skipChange )
            e->SetConeFalloff( v );
        }
      }
    }
  }
  ImGui::End();

  if ( makeArray )
  {
    if ( ImGui::Begin( "Make entity array", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
    {
      ImGui::InputFloat3( "Position increment", &arrayPosOffset.x );
      ImGui::InputFloat3( "Rotation increment", &arrayRotOffset.x );
      ImGui::InputFloat3( "Scale increment", &arraySizOffset.x );
      ImGui::InputInt( "New entity count", &arrayCount );

      if ( ImGui::Button( "Create" ) )
      {
        SigMakeArrayOfEntities( id, arrayPosOffset, arrayRotOffset, arraySizOffset, arrayCount );
        makeArray = false;
      }

      ImGui::SameLine();

      if ( ImGui::Button( "Cancel" ) )
        makeArray = false;
    }
    ImGui::End();
  }

  skipChange = false;
}

void EntityPropertiesWindow::HandleSlotEntity()
{
  auto e = dynamic_cast< ModelEntity* >( id.GetEntity() );
  if ( !gameDefinition.IsSlotEntity( e->GetGUID() ) )
    return;

  ImGui::Separator();

  ImGui::PushItemWidth( 80 );

  auto gdEntry = gameDefinition.GetEntities().find( e->GetGUID() )->second;
  auto slotIx  = -1;

  bool slotSetupChanged = false;

  for ( auto& content : gdEntry.visuals.frames.front().contents )
  {
    if ( content.type != GameDefinition::Entity::Visuals::Frame::Content::Type::Slot )
      continue;

    slotIx++;

    auto modelLabel = content.data.slot.name + L" model";
    slotSetupChanged |= ImGui::SliderInt( N( modelLabel.data() ).data(), &slotConfig[ slotIx ].modelIx, 0, int( content.data.slot.models.size() - 1 ) );
    if ( slotSetupChanged )
      slotConfig[ slotIx ].materialIx = 0;

    auto& model = std::next( content.data.slot.models.begin(), slotConfig[ slotIx ].modelIx )->second;

    ImGui::SameLine();
    auto materialLabel = content.data.slot.name + L" material";
    slotSetupChanged |= ImGui::SliderInt( N( materialLabel.data() ).data(), &slotConfig[ slotIx ].materialIx, 0, int( model.materials.size() - 1 ) );
  }

  if ( slotSetupChanged )
    SigSlotSetupChanged( id, slotConfig );
}

void EntityPropertiesWindow::SetEntity( EntityId id )
{
  if ( this->id != id )
  {
    this->id = id;
    skipChange = true;
    makeArray = false;

    if ( auto e = id.GetEntity() )
      if ( auto me = dynamic_cast< ModelEntity* >( e ) )
        if ( gameDefinition.IsSlotEntity( me->GetGUID() ) )
        {
          auto& gdEntry = gameDefinition.GetEntities().find( me->GetGUID() )->second;
          auto& esc     = me->GetSlotConfig();

          slotConfig.clear();
          for ( auto& content : gdEntry.visuals.frames.front().contents )
          {
            auto escIter = esc.find( content.data.slot.guid );
            if ( escIter == esc.end() )
              slotConfig.emplace_back( SlotEntry { 0, 0 } );
            else
            {
              auto modelIx    = 0;
              auto materialIx = 0;

              auto modelIter = content.data.slot.models.find( escIter->second.model );
              if ( modelIter != content.data.slot.models.end() )
              {
                modelIx = int( std::distance( content.data.slot.models.begin(), modelIter ) );

                auto materialIter = std::find( modelIter->second.materials.begin(), modelIter->second.materials.end(), escIter->second.material );
                if ( materialIter != modelIter->second.materials.end() )
                  materialIx = int( std::distance( modelIter->second.materials.begin(), materialIter ) );
              }

              slotConfig.emplace_back( SlotEntry{ modelIx, materialIx } );
            }
          }
        }
  }
}
