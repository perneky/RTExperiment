#include "Sandbox.h"
#include "Platform/Window.h"
#include "Render/RenderManager.h"
#include "Render/CommandList.h"
#include "Render/Swapchain.h"
#include "Render/Mesh.h"
#include "Render/Utils.h"
#include "Render/Scene.h"
#include "Render/ShaderStructuresNative.h"
#include "Common/Color.h"
#include "Common/Finally.h"
#include "Common/Files.h"
#include "Game/GameStage.h"
#include "Game/GameDefinition.h"
#include "Game/ModelEntity.h"
#include "UI/Debug/DebugWindow.h"
#include "UI/Editor/StageWindow.h"
#include "UI/Editor/EntityPropertiesWindow.h"
#include "UI/Editor/EditorMainWindow.h"
#include "UI/Editor/EntityPaletteWindow.h"
#include "UI/Editor/ToolWindow.h"
#include "../DearImGui/imgui.h"
#include "../External/tinyxml2/tinyxml2.h"

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
  gameDefinition.Load( ReadFileToMemory( L"Content\\Skies.xml" ) );
  gameDefinition.Load( ReadFileToMemory( L"Content\\IndustrialAreaHangar.xml" ) );
  gameDefinition.Load( ReadFileToMemory( L"Content\\Mercs.xml" ) );

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  ImGui::StyleColorsDark();

  std::shared_ptr< Window > window = Window::Create( 1280, 720, false );
  if ( RenderManager::CreateInstance( window ) )
  {
    {
      std::unique_ptr< GameStage > stage;
      EntityId                     cameraEntity;

      auto&    renderManager       = RenderManager::GetInstance();
      uint64_t nextFrameFenceValue = 0;

      auto commandAllocator = renderManager.RequestCommandAllocator( CommandQueueType::Direct );
      auto commandList      = renderManager.CreateCommandList( commandAllocator, CommandQueueType::Direct );

      renderManager.SetUp( *commandList );
      renderManager.RecreateWindowSizeDependantResources( *commandList );
      renderManager.PrepareAllMaterials( *commandList );

      bool windowResized = false;
      window->SigSizeChanged.Connect( [&]( int, int )
      {
        windowResized = true;
      } );

      auto     lastFrameTime = GetCPUTime();
      uint64_t frameCounter  = 0;

      DebugWindow            debugWindow;
      StageWindow            stageWindow;
      EntityPropertiesWindow entityPropertiesWindow;
      EntityPaletteWindow    entityPaletteWindow;
      ToolWindow             toolWindow( *commandList );
      EditorMainWindow       editorMainWindow( *window );

      renderManager.Submit( std::move( commandList ), CommandQueueType::Direct, true );

      std::optional< std::wstring > stageRequest;

      auto P = [&cameraEntity]( EntityId id ) 
      {
        if ( auto c = cameraEntity.GetEntity() )
        {
          if ( auto e = id.GetEntity() )
          {
            auto cameraWorld   = c->GetWorldTransform();
            auto cameraForward = cameraWorld.r[ 2 ];
            e->SetPosition( XMVectorMultiplyAdd( cameraForward, XMVectorSet( 2, 2, 2, 1 ), c->GetPosition() ) );
          }
        }

        return id;
      };

      editorMainWindow.SigStageChangeRequested.Connect( [&stageRequest]( const wchar_t* path )
      {
        stageRequest = std::wstring( path ? path : L"" );
      } );

      EntityId entityToDuplicate;
      int      duplicateCount = 0;
      XMFLOAT3 duplicateIncPosition;
      XMFLOAT3 duplicateIncRotation;
      XMFLOAT3 duplicateIncScale;
      entityPropertiesWindow.SigDuplicateEntity.Connect( [&]( EntityId id )
      {
        entityToDuplicate    = id;
        duplicateCount       = 1;
        duplicateIncPosition = { 0, 0, 0 };
        duplicateIncRotation = { 0, 0, 0 };
        duplicateIncScale    = { 0, 0, 0 };
      } );

      entityPropertiesWindow.SigMakeArrayOfEntities.Connect( [&]( EntityId id, XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale, int count )
      {
        if ( id == EntityId() || count < 1 )
          return;

        entityToDuplicate    = id;
        duplicateCount       = count;
        duplicateIncPosition = pos;
        duplicateIncRotation = rot;
        duplicateIncScale    = scale;
      } );

      EntityId entityWithSlotsChanged;
      std::vector< EntityPropertiesWindow::SlotEntry > newSlotConfig;
      entityPropertiesWindow.SigSlotSetupChanged.Connect( [&]( EntityId id, std::vector< EntityPropertiesWindow::SlotEntry > config )
      {
        entityWithSlotsChanged = id;
        newSlotConfig          = config;
      } );

      entityPaletteWindow.SigRemoveSelectedEntity.Connect( [&stage]()
      {
        if ( stage && stage->GetSelected() )
          stage->RemoveEntity( stage->GetSelected() );
      } );

      entityPaletteWindow.SigPlacePointLight.Connect( [&stage, &P]()
      {
        if ( stage )
          stage->SetSelected( P( stage->AddLightEntity( LightTypeCB::Point ) ) );
      } );

      entityPaletteWindow.SigPlaceSpotLight.Connect( [&stage, &P]()
      {
        if ( stage )
          stage->SetSelected( P( stage->AddLightEntity( LightTypeCB::Spot ) ) );
      } );

      GUID modelToPlace = {};
      entityPaletteWindow.SigPlaceModel.Connect( [&modelToPlace]( const GUID& guid )
      {
        modelToPlace = guid;
      } );

      auto shoudQuit = false;
      while ( !shoudQuit )
      {
        shoudQuit = !window->ProcessMessages();

        auto commandAllocator = renderManager.RequestCommandAllocator( CommandQueueType::Direct );
        auto commandList      = renderManager.CreateCommandList( commandAllocator, CommandQueueType::Direct );

        if ( stage )
          stage->GetScene()->SetUpscalingQuality( *commandList, *window, debugWindow.GetUpscalingQuality() );

        if ( windowResized )
        {
          renderManager.IdleGPU();

          renderManager.GetSwapchain().Resize( *commandList, renderManager.GetDevice(), *window );
          if ( stage )
            stage->RecreateWindowSizeDependantResources( *commandList, *window );
          windowResized = false;

          renderManager.IdleGPU();
        }

        if ( stageRequest.has_value() )
        {
          renderManager.IdleGPU();
          stage.reset();
          renderManager.IdleGPU();

          if ( stageRequest->empty() )
          {
            stage = std::make_unique< GameStage >( *commandList, *window );
            stage->SetSky( *commandList, ParseGUID( "{ED542769-36A3-4AC8-ADE5-8E08E2F1EB32}" ) );

            cameraEntity = stage->AddCameraEntity();
            Sandbox::SetupCameraControl( *stage, *cameraEntity.GetEntity(), *window, toolWindow );

            stageWindow.SetStage( stage.get() );
            editorMainWindow.SetStage( stage.get(), L"" );
          }
          else
          {
            FILE* file = nullptr;
            if ( _wfopen_s( &file, stageRequest->data(), L"rb" ) == 0 )
            {
              tinyxml2::XMLDocument doc;
              doc.LoadFile( file );
              auto rootElement      = doc.RootElement();
              auto gameStageElement = rootElement->FirstChildElement( "GameStage" );

              stage = std::make_unique< GameStage >( *commandList, *window );

              if ( gameStageElement )
                stage->Deserialize( *commandList, *gameStageElement );
              else
                stage.reset();

              cameraEntity = stage->GetLastUsedCameraEntityId();
              Sandbox::SetupCameraControl( *stage, *cameraEntity.GetEntity(), *window, toolWindow );

              stageWindow.SetStage( stage.get() );
              editorMainWindow.SetStage( stage.get(), stageRequest->data() );

              fclose( file );
            }
          }

          stageRequest.reset();
        }

        auto& backBuffer = renderManager.GetSwapchain().GetCurrentBackBufferTexture();

        auto thisFrameTime = GetCPUTime();
        auto timeElapsed = thisFrameTime - lastFrameTime;
        lastFrameTime = thisFrameTime;

        Sandbox::Tick( *commandList, float( timeElapsed ) );

        renderManager.GetDevice().DearImGuiNewFrame();
        window->DearImGuiNewFrame();
        ImGui::NewFrame();

        renderManager.PrepareNextSwapchainBuffer( nextFrameFenceValue );

        if ( stage )
        {
          if ( modelToPlace != GUID {} )
          {
            stage->SetSelected( P( stage->AddModelEntity( *commandList, modelToPlace ) ) );
            modelToPlace = GUID {};
          }

          if ( entityToDuplicate != EntityId() )
          {
            auto entity  = entityToDuplicate.GetEntity();
            auto basePos = entity->GetPosition();
            auto baseRot = entity->GetRotation();
            auto baseSiz = entity->GetScale();
            auto posInc  = XMLoadFloat3( &duplicateIncPosition );
            auto rotInc  = XMLoadFloat3( &duplicateIncRotation );
            auto sizInc  = XMLoadFloat3( &duplicateIncScale );
            for ( int inst = 1; inst <= duplicateCount; ++inst )
            {
              auto newId = stage->Duplicate( *commandList, entityToDuplicate );
              assert( newId != EntityId() );

              auto newEntity = newId.GetEntity();
              newEntity->SetPosition( basePos + posInc * float( inst ) );
              newEntity->SetRotation( baseRot + rotInc * float( inst ) );
              newEntity->SetScale   ( baseSiz + sizInc * float( inst ) );

              if ( inst == duplicateCount )
                stage->SetSelected( newId );
            }
            entityToDuplicate = EntityId();
          }

          if ( entityWithSlotsChanged != EntityId() )
          {
            auto  entity      = entityWithSlotsChanged.GetEntity();
            auto  modelEntity = dynamic_cast< ModelEntity* >( entity );
            auto& gdEntry     = gameDefinition.GetEntities().find( modelEntity->GetGUID() )->second;
            auto& contents    = gdEntry.visuals.frames.front().contents;

            SlotConfig sc = modelEntity->GetSlotConfig();
            for ( auto sceIx = 0; sceIx < newSlotConfig.size(); ++sceIx )
            {
              auto slot     = std::next( contents.begin(), sceIx );
              auto model    = std::next( slot->data.slot.models.begin(), newSlotConfig[ sceIx ].modelIx );
              auto material = std::next( model->second.materials.begin(), newSlotConfig[ sceIx ].materialIx );

              sc[ slot->data.slot.guid ] = SlotEntry { model->first, *material };
            }

            modelEntity->SetSlotConfig( *commandList, std::move( sc ) );

            newSlotConfig.clear();
            entityWithSlotsChanged = EntityId();
          }

          stage->Update( *commandList, timeElapsed );

          Sandbox::TickCamera( *cameraEntity.GetEntity(), float( timeElapsed ) );

          GameStage::EditorInfo editorInfo;
          editorInfo.frameDebugMode         = debugWindow.GetFrameDebugMode();
          editorInfo.activeGizmo            = toolWindow.GetActiveGizmo();
          editorInfo.renderLightMarkers     = editorMainWindow.ShouldShowLightMarkers();
          editorInfo.showGIProbes           = debugWindow.GetShowGIProbes();
          editorInfo.showLuminanceHistogram = debugWindow.GetShowLuminanceHistogram();

          auto sceneColorAndDepthTexture = stage->Render( *commandList, cameraEntity, float( timeElapsed ), editorInfo );
          
          commandList->BeginEvent( 251, L"Copy to back buffer" );
          commandList->CopyResource( sceneColorAndDepthTexture.first, backBuffer );
          commandList->EndEvent();
        }
        else
        {
          commandList->ChangeResourceState( backBuffer, ResourceStateBits::RenderTarget );
          commandList->Clear( backBuffer, Color() );
        }

        {
          commandList->BeginEvent( 0, L"DearImGui" );

          commandList->ChangeResourceState( backBuffer, ResourceStateBits::RenderTarget );
          commandList->SetRenderTarget( backBuffer, nullptr );

          if ( stage )
          {
            toolWindow.SetGameStage( stage.get() );

            entityPropertiesWindow.SetEntity( stage->GetSelected() );
            entityPropertiesWindow.Tick( *commandList, timeElapsed );
            entityPaletteWindow.Tick( *commandList, timeElapsed );
            toolWindow.Tick( *commandList, timeElapsed );
            stageWindow.Tick( *commandList, timeElapsed );
          }

          editorMainWindow.Tick( *commandList, timeElapsed );
          debugWindow.Tick( *commandList, timeElapsed );

          ImGui::Render();

          commandList->DearImGuiRender( renderManager.GetDevice() );

          commandList->EndEvent();
        }

        commandList->ChangeResourceState( backBuffer, ResourceStateBits::Present );
        auto fenceValue = renderManager.Submit( std::move( commandList ), CommandQueueType::Direct, false );
        renderManager.DiscardCommandAllocator( CommandQueueType::Direct, commandAllocator, fenceValue );
        nextFrameFenceValue = renderManager.Present( fenceValue );

        renderManager.TidyUp();

        if ( shoudQuit )
        {
          if ( stage )
          {
            auto commandAllocator = renderManager.RequestCommandAllocator( CommandQueueType::Direct );
            auto commandList      = renderManager.CreateCommandList( commandAllocator, CommandQueueType::Direct );

            stage->TearDown( *commandList );

            auto fenceValue = renderManager.Submit( std::move( commandList ), CommandQueueType::Direct, false );
            renderManager.DiscardCommandAllocator( CommandQueueType::Direct, commandAllocator, fenceValue );
            renderManager.TidyUp();
          }
        }
        else
          renderManager.GetDevice().StartNewFrame();
      }

      renderManager.IdleGPU();
    }

    RenderManager::DeleteInstance();
  }

  ImGui::DestroyContext();

  return 0;
}
