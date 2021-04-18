#include "ModelEntity.h"
#include "GameDefinition.h"
#include "GameStage.h"
#include "AnimationPlayer.h"
#include "Render/RenderManager.h"
#include "Render/Scene.h"
#include "Render/Device.h"
#include "Render/DescriptorHeap.h"
#include "Render/Resource.h"
#include "Render/ResourceDescriptor.h"
#include "Render/Mesh.h"
#include "Render/CommandList.h"
#include "Render/RTBottomLevelAccelerator.h"
#include "../External/tinyxml2/tinyxml2.h"

struct ModelEntityHelper
{
  static void AddFrame( ModelEntity& modelEntity, CommandList& commandList, Scene& scene, const GameDefinition::Entity& entity, bool isSlotEntity, const GameDefinition::Entity::Visuals::Frame& frame, CXMMATRIX worldTransform )
  {
    std::map< std::wstring, std::set< int > > modelMap;
    for ( auto& content : frame.contents )
    {
      assert( content.type == GameDefinition::Entity::Visuals::Frame::Content::Type::Mesh );
      modelMap[ content.source ].insert( content.data.mesh.subsets.begin(), content.data.mesh.subsets.end() );
    }

    for ( auto& model : modelMap )
    {
      modelEntity.subsets.insert( model.second.begin(), model.second.end() );

      auto meshId = scene.AddMesh( commandList, model.first.data(), isSlotEntity, worldTransform, std::move( model.second ), [&]( int sectionIndex, int localMaterialIndex, int& materialIndex, bool& twoSided, AlphaModeCB& alphaMode )
      {
        auto localMaterialIter = entity.visuals.materials.find( sectionIndex );
        if ( localMaterialIter == entity.visuals.materials.end() )
        {
          materialIndex = 0;
          twoSided      = false;
          alphaMode     = AlphaModeCB::Opaque;
        }
        else
        {
          auto materialIter = gameDefinition.GetEntityMaterials().find( localMaterialIter->second );
          assert( materialIter != gameDefinition.GetEntityMaterials().end() );

          materialIndex = RenderManager::GetInstance().GetAllMaterialIndex( materialIter->first );
          twoSided      = materialIter->second.twoSided;
          alphaMode     = ConvertToAlphaMode( materialIter->second.blendMode );
        }

      } );
      modelEntity.sceneMeshes.emplace_back( meshId );
    }

    for ( auto& child : frame.children )
      AddFrame( modelEntity, commandList, scene, entity, isSlotEntity, child, worldTransform );
  }
};

SlotConfig ParseSlotConfig( const tinyxml2::XMLElement* configNode )
{
  SlotConfig result;

  if ( !configNode )
    return result;

  for ( auto slotNode = configNode->FirstChildElement( "Slot" ); slotNode; slotNode = slotNode->NextSiblingElement( "Slot" ) )
    result[ ParseGUID( slotNode->Attribute( "id" ) ) ] = SlotEntry { ParseGUID( slotNode->Attribute( "model" ) )
                                                                   , ParseGUID( slotNode->Attribute( "material" ) ) };

  return result;
}

std::unique_ptr< ModelEntity > ModelEntity::CreateDeserialize( tinyxml2::XMLNode& node, CommandList& commandList, GameStage& stage, Scene& scene )
{
  using namespace tinyxml2;

  auto guidAttribute = static_cast< XMLElement* >( &node )->FindAttribute( "guid" );
  assert( guidAttribute );

  auto entity = std::make_unique< ModelEntity >( commandList
                                               , stage
                                               , scene
                                               , ParseGUID( guidAttribute->Value() )
                                               , ParseSlotConfig( node.FirstChildElement( "SlotConfig" ) ) );
  entity->Deserialize( node );
  return entity;
}

ModelEntity::ModelEntity( CommandList& commandList, GameStage& stage, Scene& scene, const GUID& guid, SlotConfig&& slotConfig )
  : scene( &scene )
  , stage( &stage )
  , guid( guid )
  , slotConfig( std::forward< SlotConfig >( slotConfig ) )
{
  AddToScene( commandList );
}

ModelEntity::~ModelEntity()
{
  RemoveFromScene();
}

float ModelEntity::Pick( FXMVECTOR rayStart, FXMVECTOR rayDir ) const
{
  for ( auto id : sceneMeshes )
  {
    float d = scene->Pick( id, rayStart, rayDir );
    if ( d >= 0 )
      return d;
  }

  return -1;
}

void ModelEntity::Serialize( uint32_t id, tinyxml2::XMLNode& node )
{
  using namespace tinyxml2;

  auto entityElement = static_cast< XMLElement* >( node.InsertEndChild( node.GetDocument()->NewElement( "ModelEntity" ) ) );
  entityElement->SetAttribute( "guid", to_string( guid ).data() );

  if ( animationPlayer )
    static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "Animation" ) ) )->SetAttribute( "name", N( animationPlayer->GetCurrentAnimation() ).data() );

  if ( !slotConfig.empty() )
  {
    auto slotConfigElement = static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "SlotConfig" ) ) );
    for ( auto& config : slotConfig )
    {
      auto slotElement = static_cast< XMLElement* >( slotConfigElement->InsertEndChild( node.GetDocument()->NewElement( "Slot" ) ) );
      slotElement->SetAttribute( "id", to_string( config.first ).data() );
      slotElement->SetAttribute( "model", to_string( config.second.model ).data() );
      slotElement->SetAttribute( "material", to_string( config.second.material ).data() );
    }
  }

  Entity::Serialize( id, *entityElement );
}

void ModelEntity::Deserialize( tinyxml2::XMLNode& node )
{
  using namespace tinyxml2;

  Entity::Deserialize( node );

  const char* animName = nullptr;
  if ( animationPlayer )
    if ( auto animNode = node.FirstChildElement( "Animation" ) )
      if ( animNode->QueryStringAttribute( "name", &animName ) == XMLError::XML_SUCCESS )
        animationPlayer->SetCurrentAnimation( W( animName ).data() );
}

void ModelEntity::Update( CommandList& commandList, double timeElapsed )
{
  if ( animationPlayer )
  {
    commandList.BeginEvent( 0, L"ModelEntity::Update(Animation)" );

    animationPlayer->Update( timeElapsed );

    auto& renderManager = RenderManager::GetInstance();
    auto& device        = renderManager.GetDevice();
    auto  mesh          = scene->GetMesh( sceneMeshes.front() );
    auto& skinVertices  = mesh->GetVertexBufferResource();
    auto& materials     = mesh->GetPackedMaterialIndices();
    auto  vertexCount   = mesh->GetVertexCount();

    auto skinVerticesDesc = skinVertices.GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView );
    if ( !skinVerticesDesc )
    {
      auto desc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, -1, skinVertices, sizeof( SkinnedVertexFormat ) );
      skinVerticesDesc = desc.get();
      skinVertices.AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( desc ) );
    }

    auto materialsDesc = materials.GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView );
    if ( !materialsDesc )
    {
      auto desc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, -1, materials, sizeof( uint32_t ) );
      materialsDesc = desc.get();
      materials.AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( desc ) );
    }

    auto& skinTransformBuffer = renderManager.GetSkinTransformBuffer();

    commandList.ChangeResourceState( materials, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( skinTransformBuffer, ResourceStateBits::VertexOrConstantBuffer );
    commandList.ChangeResourceState( *transformedSkinVertices, ResourceStateBits::UnorderedAccess );
    commandList.ChangeResourceState( *transformedSkinRTVertexBuffer, ResourceStateBits::UnorderedAccess );

    commandList.SetComputeShader( RenderManager::GetInstance().GetSkinTransformShader() );
    commandList.SetComputeResource( 1, *materialsDesc );
    commandList.SetComputeResource( 4, *skinVerticesDesc );
    commandList.SetComputeResource( 5, *transformedSkinVertices->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.SetComputeResource( 6, *transformedSkinRTVertexBuffer->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );

    // TODO: go through each subset, and compute the batches one-by-one using the index
    // buffer and batch ranges. Generate only the vertices being used. Use that for both RT
    // and raster
    for ( auto subset : subsets )
    {
      auto indexRange = mesh->GetIndexRangeForSubset( subset );

      static XMFLOAT4X4 boneXforms[ 256 ];
      auto& boneNames = mesh->GetBoneNamesForSubset( subset );
      animationPlayer->GetBoneTransforms( boneNames, boneXforms );

      auto ub = renderManager.GetUploadConstantBufferForResource( skinTransformBuffer );
      commandList.UploadBufferResource( std::move( ub ), skinTransformBuffer, boneXforms, int( sizeof( boneXforms ) ) );

      auto& skinIndices = mesh->GetIndexBufferResourceForSubset( subset );
      auto skinIndicesDesc = skinIndices.GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView );

      struct
      {
        int indexCount;
        int vertexCount;
      } subsetData;

      subsetData.indexCount  = indexRange.second;
      subsetData.vertexCount = vertexCount;

      commandList.SetComputeConstantValues( 0, subsetData );
      commandList.SetComputeConstantBuffer( 2, skinTransformBuffer );
      commandList.SetComputeResource( 3, *skinIndicesDesc );
      commandList.Dispatch( ( indexRange.second + SkinTransformKernelWidth - 1 ) / SkinTransformKernelWidth, 1, 1 );
    }

    commandList.AddUAVBarrier( { transformedSkinVertices.get(), transformedSkinRTVertexBuffer.get() } );

    commandList.ChangeResourceState( *transformedSkinVertices, ResourceStateBits::VertexOrConstantBuffer | ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *transformedSkinRTVertexBuffer, ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );

    for ( auto subset : subsets )
    {
      auto  indexRange  = mesh->GetIndexRangeForSubset( subset );
      auto& skinIndices = mesh->GetIndexBufferResourceForSubset( subset );

      if ( skinRTAccelerators[ subset ] )
        skinRTAccelerators[ subset ]->Update( device, commandList, *transformedSkinVertices, skinIndices );
      else
      {
        auto blas = device.CreateRTBottomLevelAccelerator( commandList, *transformedSkinVertices, skinVertexCount, 16, sizeof( RigidVertexFormatWithHistory ), skinIndices, 16, indexRange.second, true, true );
        scene->SetRayTracingForMesh( sceneMeshes.front(), subset, blas.get() );
        skinRTAccelerators[ subset ] = std::move( blas );
      }
    }

    scene->SetVertexBufferForMesh( sceneMeshes.front(), transformedSkinVertices.get() );

    commandList.EndEvent();
  }
}

std::unique_ptr<Entity> ModelEntity::Duplicate( CommandList& commandList )
{
  auto model = std::make_unique< ModelEntity >( commandList, *stage, *scene, guid );

  model->position = position;
  model->rotation = rotation;
  model->scale    = scale;

  model->worldTransform = worldTransform;

  model->TransformChanged();

  return model;
}

std::vector< const wchar_t* > ModelEntity::GetAnimationNames() const
{
  if ( !animationPlayer )
    return {};

  std::vector< const wchar_t* > result;
  for ( int ix = 0; ix < animationPlayer->GetAnimationCount(); ++ix )
    result.emplace_back( animationPlayer->GetAnimationName( ix ) );
  return result;
}

const wchar_t* ModelEntity::GetCurrentAnimationName() const
{
  return animationPlayer ? animationPlayer->GetCurrentAnimation() : L"";
}

void ModelEntity::SetCurrentAnimationName( const wchar_t* name )
{
  if ( animationPlayer )
    animationPlayer->SetCurrentAnimation( name );
}

std::set<uint32_t> ModelEntity::GetMeshIds() const
{
  std::set< uint32_t > meshes;
  for ( auto id : sceneMeshes )
    meshes.emplace( id );
  return meshes;
}

GUID ModelEntity::GetGUID() const
{
  return guid;
}

BoundingBox ModelEntity::CalcAABB() const
{
  BoundingBox aabb = scene->CalcModelAABB( sceneMeshes.front() );

  for ( auto id : sceneMeshes )
    BoundingBox::CreateMerged( aabb, aabb, scene->CalcModelAABB( id ) );

  return aabb;
}

const SlotConfig& ModelEntity::GetSlotConfig() const
{
  return slotConfig;
}

void ModelEntity::SetSlotConfig( CommandList& commandList, SlotConfig&& config )
{
  slotConfig.swap( config );

  RemoveFromScene();
  AddToScene( commandList );
  TransformChanged();
}

void ModelEntity::TransformChanged()
{
  for ( auto id : sceneMeshes )
    scene->MoveMesh( id, XMLoadFloat4x4( &worldTransform ) );
}

void ModelEntity::AddToScene( CommandList& commandList )
{
  auto entityIter = gameDefinition.GetEntities().find( guid );
  if ( entityIter != gameDefinition.GetEntities().end() )
  {
    GameDefinition::Entity slotEntity;
    auto isSlotEntity = gameDefinition.IsSlotEntity( guid );
    if ( isSlotEntity )
      slotEntity = gameDefinition.GenerateEntityFromSlots( guid, slotConfig );

    auto& entity = isSlotEntity ? slotEntity : entityIter->second;
    for ( auto& frame : entity.visuals.frames )
      ModelEntityHelper::AddFrame( *this, commandList, *scene, entity, isSlotEntity, frame, XMMatrixIdentity() );

    if ( !animationPlayer && entity.visuals.animationSet != GUID{} )
    {
      assert( sceneMeshes.size() == 1 );

      auto mesh = scene->GetMesh( sceneMeshes.front() );
      assert( mesh );

      skinVertexCount = mesh->GetVertexCount();

      animationPlayer = std::make_unique< AnimationPlayer >( entity.visuals.animationSet );

      auto& device = RenderManager::GetInstance().GetDevice();

      int   vs     = sizeof( RigidVertexFormatWithHistory );
      int   bs     = vs * skinVertexCount;
      transformedSkinVertices = device.CreateBuffer( ResourceType::VertexBuffer, HeapType::Default, true, bs, vs, L"MeshTransformedSkinVertices" );
      auto desc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::UnorderedAccessView, -1, *transformedSkinVertices, vs );
      transformedSkinVertices->AttachResourceDescriptor( ResourceDescriptorType::UnorderedAccessView, std::move( desc ) );

      int rtvs   = sizeof( RTVertexFormat );
      int rtbs   = rtvs * skinVertexCount;
      int rtslot = mesh->GetRTVertexSlot();
      transformedSkinRTVertexBuffer = device.CreateBuffer( ResourceType::VertexBuffer, HeapType::Default, true, rtbs, rtvs, L"SkinnedVBRT" );
      desc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::UnorderedAccessView, -1, *transformedSkinRTVertexBuffer, rtvs );
      transformedSkinRTVertexBuffer->AttachResourceDescriptor( ResourceDescriptorType::UnorderedAccessView, std::move( desc ) );
      desc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, rtslot, *transformedSkinRTVertexBuffer, rtvs );
      transformedSkinRTVertexBuffer->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( desc ) );
    }
  }
}

void ModelEntity::RemoveFromScene()
{
  for ( auto id : sceneMeshes )
    scene->RemoveMesh( id );
  sceneMeshes.clear();
}
