#include "GameDefinition.h"
#include "tinyxml2.h"

using namespace tinyxml2;

GUID InvalidGUID = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };

static GameDefinition::Sky ParseSky( XMLElement* element )
{
  GameDefinition::Sky sky;

  ParseGUID( element->Attribute( "guid" ), sky.guid );
  sky.name           = W( element->Attribute( "name" ) );
  sky.material       = ParseGUID( element->Attribute( "material" ) );
  sky.lightIntensity = element->FloatAttribute( "lightIntensity", 1 );
  ToVector3( element->Attribute( "lightColor" ), sky.lightColor );
  ToVector3( element->Attribute( "lightDirection" ), sky.lightDirection );

  return sky;
}

static GameDefinition::EntityBrush ParseEntityBrush( XMLElement* element )
{
  GameDefinition::EntityBrush brush;

  ParseGUID( element->Attribute( "guid" ), brush.guid );
  brush.name = W( element->Attribute( "name" ) );

  for ( auto entityNode = element->FirstChildElement( "Entity" ); entityNode; entityNode = entityNode->NextSiblingElement( "Entity" ) )
  {
    GUID guid = InvalidGUID;
    ParseGUID( entityNode->Attribute( "guid" ), guid );
    brush.entities[ guid ] = entityNode->IntAttribute( "chance" );
  }

  return brush;
}

static GameDefinition::AnimationSet ParseAnimationSet( XMLElement* element )
{
  GameDefinition::AnimationSet animSet;

  ParseGUID( element->Attribute( "guid" ), animSet.guid );
  animSet.name = W( element->Attribute( "name" ) );
  animSet.path = W( element->Attribute( "path" ) );

  for ( auto animNode = element->FirstChildElement( "Animation" ); animNode; animNode = animNode->NextSiblingElement( "Animation" ) )
  {
    animSet.animations.emplace_back();
    animSet.animations.back().name  = W( animNode->Attribute( "name" ) );
    animSet.animations.back().group = W( animNode->Attribute( "group" ) );
    animSet.animations.back().start = animNode->IntAttribute( "start" );
    animSet.animations.back().end   = animNode->IntAttribute( "end" );
  }

  return animSet;
}

static GameDefinition::Material ParseMaterial( XMLElement* element )
{
  GameDefinition::Material material;

  std::wstring basePath = W( element->Attribute( "basePath" ) );
  if ( !basePath.empty() )
    basePath += '\\';

  ParseGUID( element->Attribute( "guid" ), material.guid );
  material.name = W( element->Attribute( "name" ) );
  if ( auto e = element->FirstChildElement( "Thumbnail" ) )
    material.thumbnailPath = W( e->Attribute( "path" ) );
  if ( auto e = element->FirstChildElement( "ColorChannel" ) )
    material.colorMapPath = basePath + W( e->Attribute( "path" ) );
  if ( auto e = element->FirstChildElement( "NormalChannel" ) )
  {
    material.normalMapPath = basePath + W( e->Attribute( "path" ) );
    if ( e->Attribute( "flipX" ) != nullptr )
      material.flipNormalX = true;
  }
  if ( auto e = element->FirstChildElement( "RoughnessChannel" ) )
  {
    material.roughnessMapPath = basePath + W( e->Attribute( "path" ) );
    material.roughnessValue   = e->FloatAttribute( "value", 0 );
  }
  if ( auto e = element->FirstChildElement( "MetallicChannel" ) )
  {
    material.metallicMapPath = basePath + W( e->Attribute( "path" ) );
    material.metallicValue   = e->FloatAttribute( "value", 0 );
  }
  if ( auto e = element->FirstChildElement( "SpecularChannel" ) )
  {
    material.roughnessMapPath = basePath + W( e->Attribute( "path" ) );
    material.roughnessValue = e->FloatAttribute( "value", 0 );
    material.useSpecular = true;
  }
  if ( auto e = element->FirstChildElement( "EmissiveChannel" ) )
    material.emissiveMapPath = basePath + W( e->Attribute( "path" ) );

  if ( auto e = element->FirstChildElement( "Sampler" ) )
  {
    auto sampling = e->Attribute( "sampling" );
    auto wrapping = e->Attribute( "wrapping" );
    auto scaleuv  = e->Attribute( "scaleuv"  );

    if ( wrapping && _stricmp( wrapping, "clamp" ) == 0 )
      material.clampToEdge = true;

    if ( scaleuv && _stricmp( scaleuv, "yes" ) == 0 )
      material.scaleuv = true;
  }

  if ( auto e = element->FirstChildElement( "Raster" ) )
  {
    auto twosided = e->Attribute( "twosided" );
    if ( twosided && _stricmp( twosided, "yes" ) == 0 )
      material.twoSided = true;
  }

  if ( auto e = element->FirstChildElement( "Shader" ) )
  {
    auto wetness = e->Attribute( "wetness" );
    if ( wetness && _stricmp( wetness, "yes" ) == 0 )
      material.useWetness = true;
  }

  if ( auto e = element->FirstChildElement( "BlendMode" ) )
  {
    auto type = e->Attribute( "type" );
    if ( type && _stricmp( type, "Blend" ) == 0 )
      material.blendMode = BlendPreset::Blend;
    if ( type && _stricmp( type, "Additive" ) == 0 )
      material.blendMode = BlendPreset::Additive;
    if ( type && _stricmp( type, "AlphaTest" ) == 0 )
      material.blendMode = BlendPreset::AlphaTest;
  }

  return material;
}

static void ParseProjector( XMLElement* element, GameDefinition::Projector& projector )
{
  projector.path = W( element->Attribute( "path" ) );

  for ( auto imageNode = element->FirstChildElement( "Image" ); imageNode; imageNode = imageNode->NextSiblingElement( "Image" ) )
  {
    GameDefinition::Projector::Image image;

    ParseGUID( imageNode->Attribute( "guid" ), image.guid );
    image.name = W( imageNode->Attribute( "name" ) );
    image.index = imageNode->IntAttribute( "index" );

    projector.images[ image.guid ] = image;
  }
}

static GameDefinition::Entity::Visuals::Frame ParseFrame( XMLElement* element, GameDefinition::Entity::Visuals::Frame* parentFrame, const std::wstring& basePath )
{
  GameDefinition::Entity::Visuals::Frame frame;

  frame.name = W( element->Attribute( "name" ) );

  XMFLOAT3 scale      ( 1, 1, 1 );
  XMFLOAT3 rotation   ( 0, 0, 0 );
  XMFLOAT3 translation( 0, 0, 0 );

  ToVector3( element->Attribute( "scale"       ), scale       );
  ToVector3( element->Attribute( "rotation"    ), rotation    );
  ToVector3( element->Attribute( "translation" ), translation );

  auto _transform = XMMatrixScaling( scale.x, scale.y, scale.z ) 
                  * XMMatrixRotationRollPitchYaw( XMConvertToRadians( rotation.x ), XMConvertToRadians( rotation.y ), XMConvertToRadians( rotation.z ) )
                  * XMMatrixTranslation( translation.x, translation.y, translation.z );

  if ( parentFrame )
  {
    auto _parentTransform = XMLoadFloat4x4( &parentFrame->transform );
    _transform = _parentTransform * _transform;
  }

  XMStoreFloat4x4( &frame.transform, _transform );

  for ( auto meshNode = element->FirstChildElement( "Mesh" ); meshNode; meshNode = meshNode->NextSiblingElement( "Mesh" ) )
  {
    GameDefinition::Entity::Visuals::Frame::Content content;
    content.type   = GameDefinition::Entity::Visuals::Frame::Content::Type::Mesh;
    content.source = basePath + W( meshNode->Attribute( "path" ) );

    auto subsets = W( meshNode->Attribute( "submesh" ) );
    auto ranges = Split( subsets, L';' );
    for ( const auto& range : ranges )
      content.data.mesh.subsets.emplace( _wtoi( range.data() ) );

    frame.contents.emplace_back( content );
  }

  for ( auto slotNode = element->FirstChildElement( "Slot" ); slotNode; slotNode = slotNode->NextSiblingElement( "Slot" ) )
  {
    GameDefinition::Entity::Visuals::Frame::Content content;
    content.type = GameDefinition::Entity::Visuals::Frame::Content::Type::Slot;

    ParseGUID( slotNode->Attribute( "guid" ), content.data.slot.guid );
    content.data.slot.name = W( slotNode->Attribute( "name" ) );

    for ( auto meshNode = slotNode->FirstChildElement( "Mesh" ); meshNode; meshNode = meshNode->NextSiblingElement( "Mesh" ) )
    {
      ModelSlot slot;

      ParseGUID( meshNode->Attribute( "guid" ), slot.guid );
      slot.path   = basePath + W( meshNode->Attribute( "path" ) );
      slot.subset = atoi( meshNode->Attribute( "submesh" ) );

      for ( auto matNode = meshNode->FirstChildElement( "MaterialRef" ); matNode; matNode = matNode->NextSiblingElement( "MaterialRef" ) )
      {
        GUID guid;
        ParseGUID( matNode->Attribute( "guid" ), guid );
        slot.materials.emplace_back( guid );
      }

      content.data.slot.models.emplace( slot.guid, slot );
    }

    frame.contents.emplace_back( content );
  }

  for ( auto lightNode = element->FirstChildElement( "PointLight" ); lightNode; lightNode = lightNode->NextSiblingElement( "PointLight" ) )
  {
    GameDefinition::Entity::Visuals::Frame::Content content;
    content.type = GameDefinition::Entity::Visuals::Frame::Content::Type::PointLight;
    content.data.light.color    = XMFLOAT3( 1, 1, 1 );
    content.data.light.minRange = 1;
    content.data.light.maxRange = 2;
    content.data.light.exponent = 1;
    content.data.light.isPoint  = true;

    lightNode->QueryFloatAttribute( "minRange", &content.data.light.minRange );
    lightNode->QueryFloatAttribute( "maxRange", &content.data.light.maxRange );
    lightNode->QueryFloatAttribute( "exponent", &content.data.light.exponent );
    ToColor( lightNode->Attribute( "color" ), content.data.light.color );

    frame.contents.emplace_back( content );
  }

  for ( auto frameNode = element->FirstChildElement( "Frame" ); frameNode; frameNode = frameNode->NextSiblingElement( "Frame" ) )
    frame.children.emplace_back( ParseFrame( frameNode, &frame, basePath ) );

  return frame;
}

static GameDefinition::Entity ParseEntity( XMLElement* element )
{
  GameDefinition::Entity entity;

  ParseGUID( element->Attribute( "guid" ), entity.guid );
  entity.name     = W( element->Attribute( "name" ) );
  entity.category = Split( W( element->Attribute( "category" ) ), L'/' );

  if ( auto visualsNode = element->FirstChildElement( "Visuals" ) )
  {
    auto basePath = W( visualsNode->Attribute( "basePath" ) );
    if ( !basePath.empty() )
      basePath += '\\';

    if ( auto animNode = visualsNode->FirstChildElement( "AnimationSet" ) )
      ParseGUID( animNode->Attribute( "guid" ), entity.visuals.animationSet );
    else
      entity.visuals.animationSet = InvalidGUID;

    for ( auto materialNode = visualsNode->FirstChildElement( "MaterialRef" ); materialNode; materialNode = materialNode->NextSiblingElement( "MaterialRef" ) )
    {
      unsigned id;
      GUID     guid;

      if ( materialNode->QueryUnsignedAttribute( "id", &id ) == XML_SUCCESS )
      {
        if ( ParseGUID( materialNode->Attribute( "guid" ), guid ) )
        {
          entity.visuals.materials[ id ] = guid;
        }
      }
    }
    for ( auto frameNode = visualsNode->FirstChildElement( "Frame" ); frameNode; frameNode = frameNode->NextSiblingElement( "Frame" ) )
    {
      entity.visuals.frames.emplace_back( ParseFrame( frameNode, nullptr, basePath ) );
    }
  }

  return entity;
}

GameDefinition::GameDefinition()
{
}

std::vector< std::wstring > GameDefinition::GetCategoryFolders( const std::vector< std::wstring >& path ) const
{
  std::vector< std::wstring > result;

  auto hiplace = &entityHierarchy;
  for ( const auto& hix : path )
  {
    auto iter = hiplace->children.find( hix );
    if ( iter == hiplace->children.end() )
      return {};

    hiplace = &iter->second;
  }

  for ( const auto& folder : hiplace->children )
    result.emplace_back( folder.first );
  
  return result;
}

std::vector< GameDefinition::EntityCategory > GameDefinition::GetCategoryEntities( const std::vector< std::wstring >& path ) const
{
  std::vector< EntityCategory > result;

  auto hiplace = &entityHierarchy;
  for ( const auto& hix : path )
  {
    auto iter = hiplace->children.find( hix );
    if ( iter == hiplace->children.end() )
      return std::vector< EntityCategory >();

    hiplace = &iter->second;
  }

  for ( const auto& entity : hiplace->entities )
    result.emplace_back( EntityCategory( entity.first, entity.second ) );

  return result;
}

const std::wstring& GameDefinition::GetEntityName( const GUID& guid ) const
{
  auto iter = entities.find( guid );
  assert( iter != entities.end() );
  return iter->second.name;
}

bool GameDefinition::IsSlotEntity( const GUID& guid ) const
{
  auto iter = entities.find( guid );
  assert( iter != entities.end() );
  if ( iter == entities.end() )
    return false;

  return iter->second.visuals.frames.front().contents.front().type == Entity::Visuals::Frame::Content::Type::Slot;
}

GameDefinition::Entity GameDefinition::GenerateEntityFromSlots( const GUID& guid, SlotConfig& slotConfig ) const
{
  Entity result;

  auto iter = entities.find( guid );
  assert( iter != entities.end() );
  if ( iter == entities.end() )
    return result;

  auto& entity = iter->second;

  assert( entity.visuals.frames.size() == 1 );
  assert( entity.visuals.frames.front().children.empty() );
  for ( auto& content : entity.visuals.frames.front().contents )
  {
    assert( content.type == GameDefinition::Entity::Visuals::Frame::Content::Type::Slot );
    for ( auto& model : content.data.slot.models )
      assert( model.second.path == entity.visuals.frames.front().contents.front().data.slot.models.begin()->second.path );
  }

  result = entity;
  auto& singleContent = result.visuals.frames.front().contents.front();

  for ( auto& content : result.visuals.frames.front().contents )
  {
    if ( content.type == GameDefinition::Entity::Visuals::Frame::Content::Type::Slot )
    {
      GUID meshGUID     = content.data.slot.models.begin()->first;
      GUID materialGUID = content.data.slot.models.begin()->second.materials.front();

      auto slotEntryIter = slotConfig.find( content.data.slot.guid );
      if ( slotEntryIter == slotConfig.end() )
      {
        slotConfig.insert( { content.data.slot.guid, SlotEntry { meshGUID, materialGUID } } );
      }
      else
      {
        meshGUID     = slotEntryIter->second.model;
        materialGUID = slotEntryIter->second.material;
      }

      auto modelIter = content.data.slot.models.find( meshGUID );
      assert( modelIter != content.data.slot.models.end() );
      auto& model = modelIter->second;

      singleContent.data.mesh.subsets.insert( model.subset );
      result.visuals.materials[ model.subset ] = materialGUID;
    }
  }

  singleContent.type   = GameDefinition::Entity::Visuals::Frame::Content::Type::Mesh;
  singleContent.source = entity.visuals.frames.front().contents.front().data.slot.models.begin()->second.path;

  result.visuals.frames.front().contents.resize( 1 );
  result.visuals.frames.front().contents.shrink_to_fit();

  return result;
}

void GameDefinition::Load( const DataChunk& data )
{
  ::tinyxml2::XMLDocument document;
  auto parseResult = document.Parse( (const char*)data.data(), data.size() );
  assert( parseResult == XML_SUCCESS );

  auto rootElement = document.RootElement();
  assert( rootElement );

  if ( auto skiesBlock = rootElement->FirstChildElement( "Skies" ) )
  {
    for ( auto skyNode = skiesBlock->FirstChildElement( "Sky" ); skyNode; skyNode = skyNode->NextSiblingElement( "Sky" ) )
    {
      auto sky = ParseSky( skyNode );
      skies.emplace( sky.guid, std::move( sky ) );
    }
  }

  if ( auto animationsBlock = rootElement->FirstChildElement( "Animations" ) )
  {
    for ( auto animNode = animationsBlock->FirstChildElement( "AnimationSet" ); animNode; animNode = animNode->NextSiblingElement( "AnimationSet" ) )
    {
      auto animSet = ParseAnimationSet( animNode );
      animationSets.emplace( animSet.guid, std::move( animSet ) );
    }
  }

  if ( auto projectorBlock = rootElement->FirstChildElement( "Projectors" ) )
  {
    if ( auto cubeProjectorsNode = projectorBlock->FirstChildElement( "Cube" ) )
      ParseProjector( cubeProjectorsNode, cubeProjectors );

    if ( auto twodProjectorsNode = projectorBlock->FirstChildElement( "TwoD" ) )
      ParseProjector( twodProjectorsNode, twodProjectors );
  }

  if ( auto terrainMaterialBlock = rootElement->FirstChildElement( "TerrainMaterials" ) )
  {
    for ( auto materialNode = terrainMaterialBlock->FirstChildElement( "Material" ); materialNode; materialNode = materialNode->NextSiblingElement( "Material" ) )
    {
      auto material = ParseMaterial( materialNode );
      terrainMaterials.emplace( material.guid, std::move( material ) );
    }
  }

  if ( auto skyMaterialBlock = rootElement->FirstChildElement( "SkyMaterials" ) )
  {
    for ( auto materialNode = skyMaterialBlock->FirstChildElement( "Material" ); materialNode; materialNode = materialNode->NextSiblingElement( "Material" ) )
    {
      auto material = ParseMaterial( materialNode );
      skyMaterials.emplace( material.guid, std::move( material ) );
    }
  }

  for ( auto entityMaterialBlock = rootElement->FirstChildElement( "EntityMaterials" ); entityMaterialBlock; entityMaterialBlock = entityMaterialBlock->NextSiblingElement( "EntityMaterials" ) )
  {
    for ( auto materialNode = entityMaterialBlock->FirstChildElement( "Material" ); materialNode; materialNode = materialNode->NextSiblingElement( "Material" ) )
    {
      auto material = ParseMaterial( materialNode );
      entityMaterials.emplace( material.guid, std::move( material ) );
    }
  }

  if ( auto entitiesBlock = rootElement->FirstChildElement( "Entities" ) )
  {
    for ( auto entityNode = entitiesBlock->FirstChildElement( "Entity" ); entityNode; entityNode = entityNode->NextSiblingElement( "Entity" ) )
    {
      auto entity = ParseEntity( entityNode );
      auto iter   = entities.emplace( entity.guid, std::move( entity ) );

      EntityHierarchy* hiplace = &entityHierarchy;
      for ( const auto& hix : iter.first->second.category )
        hiplace = &hiplace->children[ hix ];

      hiplace->entities[ iter.first->second.guid ] = iter.first->second.thumbnail;
    }
  }

  if ( auto entityBrushesBlock = rootElement->FirstChildElement( "EntityBrushes" ) )
  {
    for ( auto brushNode = entityBrushesBlock->FirstChildElement( "Brush" ); brushNode; brushNode = brushNode->NextSiblingElement( "Brush" ) )
    {
      auto brush = ParseEntityBrush( brushNode );
      entityBrushes.emplace( brush.guid, std::move( brush ) );
    }
  }
}

GameDefinition::Entity::Visuals::Frame::Content& GameDefinition::Entity::Visuals::Frame::Content::operator = ( const Content & other )
{
  type   = other.type;
  source = other.source;
  memcpy_s( &data, sizeof data, &other.data, sizeof other.data );
  return *this;
}

GUID GameDefinition::EntityBrush::GetRandomEntity() const
{
  int sum = 0;
  for ( const auto& entity : entities )
    sum += entity.second;

  if ( sum < 1 )
    return InvalidGUID;

  int ix = rand() % sum;
  for ( const auto& entity : entities )
  {
    if ( ix < entity.second )
      return entity.first;

    ix -= entity.second;
  }

  return entities.rbegin()->first;
}
