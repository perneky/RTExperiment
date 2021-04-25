#include "GameStage.h"
#include "GameDefinition.h"
#include "Entity.h"
#include "ModelEntity.h"
#include "CameraEntity.h"
#include "LightEntity.h"
#include "Render/CommandList.h"
#include "Render/Camera.h"
#include "Render/Scene.h"
#include "Render/RenderManager.h"
#include "Render/Gizmo.h"
#include "../External/tinyxml2/tinyxml2.h"

GameStage::GameStage( CommandList& commandList, Window& window )
{
  ZeroMemory( &giArea, sizeof( giArea ) );

  scene = std::make_unique< Scene >();
  scene->SetUp( commandList, window );

  wetness.resize( wetnessSize.x * wetnessSize.y * wetnessDensity * wetnessDensity, 0 );
}

GameStage::~GameStage()
{
  entities.clear();
}

void GameStage::SetSky( CommandList& commandList, const GUID& guid )
{
  auto skyIter = gameDefinition.GetSkies().find( guid );
  if ( skyIter != gameDefinition.GetSkies().end() )
  {
    skyGUID = guid;
    scene->SetSky( commandList, guid );
  }
}

EntityId GameStage::AddModelEntity( CommandList& commandList, const GUID& guid )
{
  auto entityIter = gameDefinition.GetEntities().find( guid );
  if ( entityIter != gameDefinition.GetEntities().end() )
  {
    uint32_t id = entityIdGen++;
    entities.emplace( id, std::make_unique< ModelEntity >( commandList, *this, *scene, guid ) );
    return EntityId( *this, id );
  }

  return EntityId();
}

EntityId GameStage::AddCameraEntity()
{
  uint32_t id = entityIdGen++;
  entities.emplace( id, std::make_unique< CameraEntity >() );
  return EntityId( *this, id );
}

EntityId GameStage::AddLightEntity( LightTypeCB type )
{
  uint32_t id = entityIdGen++;
  auto entity = std::make_unique< LightEntity >();
  entity->SetType( type );
  entities.emplace( id, std::move( entity ) );
  return EntityId( *this, id );
}

EntityId GameStage::Duplicate( CommandList& commandList, EntityId eid )
{
  if ( eid == EntityId() )
    return EntityId();

  uint32_t id = entityIdGen++;
  auto dupe = eid.GetEntity()->Duplicate( commandList );
  entities.emplace( id, std::move( dupe ) );

  return EntityId( *this, id );
}

void GameStage::RemoveEntity( EntityId id )
{
  if ( entities.erase( id.GetId() ) > 0 )
  {
    if ( selected == id )
      selected = EntityId();
  }
}

std::pair< Resource&, Resource& > GameStage::Render( CommandList& commandList, EntityId camera, float dt, const EditorInfo& editorInfo )
{
  lastUsedCamera = camera;

  scene->UpdateRaytracing( commandList );

  std::vector< LightCB > renderLights;
  for ( auto& entity : entities )
    if ( auto lightEntity = dynamic_cast< LightEntity* >( entity.second.get() ) )
      renderLights.emplace_back( lightEntity->GetLight() );

  auto cameraEntity = static_cast< CameraEntity* >( camera.GetEntity() );
  cameraEntity->SetSize( float( scene->GetTargetWidth() ), float( scene->GetTargetHeight() ) );
  scene->SetCamera( cameraEntity->GetCamera() );
  scene->RebuildLightCB( commandList, renderLights );
  scene->SetBloomParams( commandList, bloomThreshold, bloomStrength );
  scene->SetToneMappingParams( commandList, enableAdaptation, targetLuminance, exposure, adaptationRate, minExposure, maxExposure );

  Scene::EditorInfo sceneEditorInfo;
  sceneEditorInfo.gizmoType              = GizmoType::None;
  sceneEditorInfo.frameDebugMode         = editorInfo.frameDebugMode;
  sceneEditorInfo.selectedGizmoElement   = activeGizmoElement;
  sceneEditorInfo.renderLightMarkers     = editorInfo.renderLightMarkers;
  sceneEditorInfo.showGIProbes           = editorInfo.showGIProbes;
  sceneEditorInfo.showLuminanceHistogram = editorInfo.showLuminanceHistogram;

  if ( selected )
  {
    auto entity = selected.GetEntity();
    if ( auto modelEntity = dynamic_cast< ModelEntity* >( entity ) )
      sceneEditorInfo.selection = modelEntity->GetMeshIds();

    sceneEditorInfo.gizmoType = editorInfo.activeGizmo;
    XMStoreFloat4x4( &sceneEditorInfo.gizmoTransform, XMMatrixTranslationFromVector( entity->GetPosition() ) );
  }

  return scene->Render( commandList, sceneEditorInfo, dt );
}

bool GameStage::Intersect( int x, int y, XMFLOAT3& hitPoint )
{
  if ( !lastUsedCamera )
    return false;

  auto  cameraEntity = static_cast< CameraEntity* >( lastUsedCamera.GetEntity() );
  auto& camera       = cameraEntity->GetCamera();
  auto  ray          = camera.GetRayFromScreen( x, y );

  float hitDistance = FLT_MAX;

  auto rayStart = XMLoadFloat3( &ray.first );
  auto rayDir   = XMLoadFloat3( &ray.second );

  for ( auto& entity : entities )
  {
    auto d = entity.second->Pick( rayStart, rayDir );
    if ( d >= 0 && d < hitDistance )
      hitDistance = d;
  }

  if ( hitDistance != FLT_MAX )
  {
    XMStoreFloat3( &hitPoint, rayStart + rayDir * hitDistance );
    return true;
  }

  return false;
}

void GameStage::Interact( int x, int y, bool activate )
{
  if ( !lastUsedCamera )
    return;

  auto  cameraEntity = static_cast< CameraEntity* >( lastUsedCamera.GetEntity() );
  auto& camera       = cameraEntity->GetCamera();
  auto  ray          = camera.GetRayFromScreen( x, y );

  if ( draggingGizmo )
  {
    assert( selected );

    auto gizmo = scene->GetGizmo();
    if ( !gizmo || gizmo->GetType() == GizmoType::None )
      return;

    auto entity = selected.GetEntity();
    auto result = gizmo->CalcManipulate( activeGizmoElement, camera, draggingGizmoOrigin, { x, y }, entity->GetWorldTransform() );
    switch ( gizmo->GetType() )
    {
    case GizmoType::Move:
      entity->SetPosition( entity->GetPosition() + result );
      break;
    case GizmoType::Scale:
      entity->SetScale( entity->GetScale() + result );
      break;
    case GizmoType::Rotate:
      entity->SetRotation( entity->GetRotation() + result );
      break;
    default:
      assert( false );
    }

    draggingGizmoOrigin = { x, y };

    return;
  }

  if ( selected )
  {
    auto gizmo = scene->GetGizmo();
    if ( !gizmo || gizmo->GetType() == GizmoType::None )
      return;

    activeGizmoElement = gizmo->Pick( selected.GetEntity()->GetWorldTransform()
                                    , XMLoadFloat3( &ray.first )
                                    , XMLoadFloat3( &ray.second ) );
  }

  if ( !activate )
    return;

  if ( activeGizmoElement != GizmoElement::None )
  {
    draggingGizmoOrigin = { x, y };
    draggingGizmo = true;
    return;
  }

  uint32_t hit         = 0;
  float    hitDistance = FLT_MAX;

  auto rayStart = XMLoadFloat3( &ray.first );
  auto rayDir   = XMLoadFloat3( &ray.second );

  for ( auto& entity : entities )
  {
    auto d = entity.second->Pick( rayStart, rayDir );
    if ( d >= 0 && d < hitDistance )
    {
      hit         = entity.first;
      hitDistance = d;
    }
  }

  selected = hit ? EntityId( *this, hit ) : EntityId();
}

void GameStage::SetSelected( EntityId id )
{
  selected = id;
}

EntityId GameStage::GetSelected() const
{
  return selected;
}

EntityId GameStage::GetLastUsedCameraEntityId() const
{
  return lastUsedCamera;
}

void GameStage::RecreateWindowSizeDependantResources( CommandList& commandList, Window& window )
{
  scene->RecreateWindowSizeDependantResources( commandList, window );
}

void GameStage::Serialize( tinyxml2::XMLNode& node )
{
  using namespace tinyxml2;

  auto stageElement    = static_cast< XMLElement* >( node.InsertEndChild( node.GetDocument()->NewElement( "GameStage" ) ) );
  auto skyElement      = static_cast< XMLElement* >( stageElement->InsertEndChild( stageElement->GetDocument()->NewElement( "Sky"      ) ) );
  auto postfxElement   = static_cast< XMLElement* >( stageElement->InsertEndChild( stageElement->GetDocument()->NewElement( "PostFx"   ) ) );
  auto entitiesElement = static_cast< XMLElement* >( stageElement->InsertEndChild( stageElement->GetDocument()->NewElement( "Entities" ) ) );
  auto wetnessElement  = static_cast< XMLElement* >( stageElement->InsertEndChild( stageElement->GetDocument()->NewElement( "Wetness"  ) ) );
  auto giAreaElement   = static_cast< XMLElement* >( stageElement->InsertEndChild( stageElement->GetDocument()->NewElement( "GIArea"   ) ) );

  stageElement->SetAttribute( "lastCameraId", lastUsedCamera.GetId() );
  skyElement->SetAttribute( "guid", to_string( skyGUID ).data() );
  entitiesElement->SetAttribute( "idGen", entityIdGen );

  postfxElement->SetAttribute( "enableAdaptation", enableAdaptation );
  postfxElement->SetAttribute( "targetLuminance ", targetLuminance  );
  postfxElement->SetAttribute( "exposure",         exposure         );
  postfxElement->SetAttribute( "adaptationRate",   adaptationRate   );
  postfxElement->SetAttribute( "minExposure",      minExposure      );
  postfxElement->SetAttribute( "maxExposure",      maxExposure      );
  postfxElement->SetAttribute( "bloomThreshold",   bloomThreshold   );
  postfxElement->SetAttribute( "bloomStrength",    bloomStrength    );

  wetnessElement->SetAttribute( "minx", wetnessOrigin.x );
  wetnessElement->SetAttribute( "minz", wetnessOrigin.y );
  wetnessElement->SetAttribute( "sizex", wetnessSize.x );
  wetnessElement->SetAttribute( "sizez", wetnessSize.y );
  wetnessElement->SetAttribute( "density", wetnessDensity );

  std::string wetnessValuesValue;
  wetnessValuesValue.reserve( wetness.size() * 2 + 1 );
  char wc[ 8 ];
  for ( auto w : wetness )
  {
    sprintf_s( wc, "%02X", int( w * 255 ) );
    wetnessValuesValue += wc;
  }

  wetnessElement->InsertEndChild( wetnessElement->GetDocument()->NewText( wetnessValuesValue.data() ) );

  giAreaElement->SetAttribute( "cx", giArea.Center.x );
  giAreaElement->SetAttribute( "cy", giArea.Center.y );
  giAreaElement->SetAttribute( "cz", giArea.Center.z );
  giAreaElement->SetAttribute( "ex", giArea.Extents.x );
  giAreaElement->SetAttribute( "ey", giArea.Extents.y );
  giAreaElement->SetAttribute( "ez", giArea.Extents.z );

  for ( auto& entity : entities )
    entity.second->Serialize( entity.first, *entitiesElement );
}

void GameStage::Deserialize( CommandList& commandList, tinyxml2::XMLNode& node )
{
  using namespace tinyxml2;

  if ( auto lastCameraIdAttribute = static_cast< XMLElement* >( &node )->FindAttribute( "lastCameraId" ) )
    lastUsedCamera = EntityId( *this, lastCameraIdAttribute->UnsignedValue() );

  if ( auto skyElement = node.FirstChildElement( "Sky" ) )
  {
    if ( auto guidAttribute = skyElement->FindAttribute( "guid" ) )
      SetSky( commandList, ParseGUID( guidAttribute->Value() ) );
  }

  if ( auto pfxElement = node.FirstChildElement( "PostFx" ) )
  {
    pfxElement->QueryBoolAttribute ( "enableAdaptation", &enableAdaptation );
    pfxElement->QueryFloatAttribute( "targetLuminance ", &targetLuminance  );
    pfxElement->QueryFloatAttribute( "exposure",         &exposure         );
    pfxElement->QueryFloatAttribute( "adaptationRate",   &adaptationRate   );
    pfxElement->QueryFloatAttribute( "minExposure",      &minExposure      );
    pfxElement->QueryFloatAttribute( "maxExposure",      &maxExposure      );
    pfxElement->QueryFloatAttribute( "bloomThreshold",   &bloomThreshold   );
    pfxElement->QueryFloatAttribute( "bloomStrength",    &bloomStrength    );
  }

  if ( auto wetnessElement = node.FirstChildElement( "Wetness" ) )
  {
    wetnessElement->QueryIntAttribute( "minx", &wetnessOrigin.x );
    wetnessElement->QueryIntAttribute( "minz", &wetnessOrigin.y );
    wetnessElement->QueryIntAttribute( "sizex", &wetnessSize.x );
    wetnessElement->QueryIntAttribute( "sizez", &wetnessSize.y );
    wetnessElement->QueryIntAttribute( "density", &wetnessDensity );

    auto wetnessValues = wetnessElement->GetText();
    wetness.resize( wetnessSize.x * wetnessSize.y * wetnessDensity * wetnessDensity, 0 );
    char wv[ 3 ] = { 0, 0, 0 };
    for ( auto& w : wetness )
    {
      wv[ 0 ] = *( wetnessValues++ );
      wv[ 1 ] = *( wetnessValues++ );
      w = strtol( wv, nullptr, 16 ) / 255.0f;
    }

    scene->SetupWetness( commandList, wetnessOrigin, wetnessSize, wetnessDensity, GetWetnessMap() );
  }

  if ( auto giAreaElement = node.FirstChildElement( "GIArea" ) )
  {
    giAreaElement->QueryFloatAttribute( "cx", &giArea.Center.x );
    giAreaElement->QueryFloatAttribute( "cy", &giArea.Center.y );
    giAreaElement->QueryFloatAttribute( "cz", &giArea.Center.z );
    giAreaElement->QueryFloatAttribute( "ex", &giArea.Extents.x );
    giAreaElement->QueryFloatAttribute( "ey", &giArea.Extents.y );
    giAreaElement->QueryFloatAttribute( "ez", &giArea.Extents.z );

    scene->SetGIArea( giArea );
  }

  if ( auto entitiesElement = node.FirstChildElement( "Entities" ) )
  {
    uint32_t largestId = 0;
    for ( auto entityElement = entitiesElement->FirstChildElement(); entityElement; entityElement = entityElement->NextSiblingElement() )
    {
      uint32_t id;
      if ( auto idAttribute = entityElement->FindAttribute( "id" ) )
      {
        id = idAttribute->UnsignedValue();
        std::unique_ptr< Entity > entity;

        if ( strcmp( entityElement->Name(), "ModelEntity" ) == 0 )
        {
          entity = ModelEntity::CreateDeserialize( *entityElement, commandList, *this, *scene );
        }
        else if ( strcmp( entityElement->Name(), "CameraEntity" ) == 0 )
        {
          entity = CameraEntity::CreateDeserialize( *entityElement );
        }
        else if ( strcmp( entityElement->Name(), "LightEntity" ) == 0 )
        {
          entity = LightEntity::CreateDeserialize( *entityElement );
        }

        entities.emplace( id, std::move( entity ) );

        largestId = std::max( largestId, id );
      }
    }

    entityIdGen = entitiesElement->UnsignedAttribute( "idGen", largestId + 1 );
    assert( entityIdGen > largestId );
  }
}

void GameStage::Update( CommandList& commandList, double timeElapsed )
{
  commandList.BeginEvent( 23, L"GameStage::Update(%f)", timeElapsed );

  for ( auto& entity : entities )
    entity.second->Update( commandList, timeElapsed );

  commandList.EndEvent();
}

GizmoElement GameStage::GetActiveGizmoElement() const
{
  return activeGizmoElement;
}

bool GameStage::GetDraggingGizmo() const
{
  return draggingGizmo;
}

void GameStage::ReleaseGizmo()
{
  draggingGizmo = false;
}

const XMINT2& GameStage::GetWetnessOrigin() const
{
  return wetnessOrigin;
}

const XMINT2& GameStage::GetWetnessSize() const
{
  return wetnessSize;
}

int GameStage::GetWetnessDensity() const
{
  return wetnessDensity;
}

std::vector< uint8_t > GameStage::GetWetnessMap() const
{
  std::vector< uint8_t > wetnessUint;
  wetnessUint.reserve( wetness.size() );

  for ( auto w : wetness )
    wetnessUint.emplace_back( Clamp( 0, 255, int( w * 255 ) ) );

  return wetnessUint;
}

void GameStage::ChangeWetnessArea( CommandList& commandList, int originX, int originZ, int sizeX, int sizeZ, int density )
{
  if ( wetnessOrigin.x == originX
    && wetnessOrigin.y == originZ
    && wetnessSize.x   == sizeX
    && wetnessSize.y   == sizeZ
    && wetnessDensity  == density )
    return;

  decltype( wetness ) newWetness;
  newWetness.resize( sizeX * sizeZ * density * density, 0 );

  // TODO: copy wetness values from the old one

  wetness.swap( newWetness );

  wetnessOrigin.x = originX;
  wetnessOrigin.y = originZ;
  wetnessSize.x   = sizeX;
  wetnessSize.y   = sizeZ;
  wetnessDensity  = density;

  scene->SetupWetness( commandList, wetnessOrigin, wetnessSize, wetnessDensity, GetWetnessMap() );
}

void GameStage::PaintWetness( CommandList& commandList, float cx, float cz, float strength, float radius, float exponent )
{
  auto ox = Clamp( wetnessOrigin.x, wetnessOrigin.x + wetnessSize.x, int( cx - radius - 1 ) );
  auto oz = Clamp( wetnessOrigin.y, wetnessOrigin.y + wetnessSize.y, int( cz - radius - 1 ) );
  auto ex = Clamp( wetnessOrigin.x, wetnessOrigin.x + wetnessSize.x, int( cx + radius + 1 ) );
  auto ez = Clamp( wetnessOrigin.y, wetnessOrigin.y + wetnessSize.y, int( cz + radius + 1 ) );

  for ( int z = oz; z < ez; ++z )
    for ( int dez = 0; dez < wetnessDensity; ++dez )
      for ( int x = ox; x < ex; ++x )
        for ( int dex = 0; dex < wetnessDensity; ++dex )
        {
          auto lx = ( x - wetnessOrigin.x ) * wetnessDensity + dex;
          auto lz = ( z - wetnessOrigin.y ) * wetnessDensity + dez;
          auto ix = lz * wetnessSize.x * wetnessDensity + lx;
          auto dx = ( x + float( dex ) / wetnessDensity ) - cx;
          auto dz = ( z + float( dez ) / wetnessDensity ) - cz;
          auto d  = sqrtf( dx * dx + dz * dz );
          auto t  = pow( 1.0f - Clamp( 0.0f, 1.0f, d / radius ), exponent );

          auto value = Clamp( 0.0f, 1.0f, wetness[ ix ] + strength * t );
          wetness[ ix ] = value;
        }

  scene->SetupWetness( commandList, wetnessOrigin, wetnessSize, wetnessDensity, GetWetnessMap() );
}

Scene* GameStage::GetScene()
{
  return scene.get();
}

const BoundingBox& GameStage::GetGIArea() const
{
  return giArea;
}

void GameStage::SetGIArea( const BoundingBox& area )
{
  if ( giArea.Center.x  == area.Center.x
    && giArea.Center.y  == area.Center.y 
    && giArea.Center.z  == area.Center.z
    && giArea.Extents.x == area.Extents.x
    && giArea.Extents.y == area.Extents.y
    && giArea.Extents.z == area.Extents.z )
    return;

  giArea = area;

  scene->SetGIArea( area );
}
