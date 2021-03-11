#include "Scene.h"
#include "Mesh.h"
#include "RenderManager.h"
#include "Utils.h"
#include "RTTopLevelAccelerator.h"
#include "Camera.h"
#include "Swapchain.h"
#include "ComputeShader.h"
#include "ResourceDescriptor.h"
#include "DescriptorHeap.h"
#include "Platform/Window.h"
#include "Common/Color.h"
#include "Common/Files.h"
#include "Icosahedron.h"
#include "Gizmo.h"

static constexpr int BlurParamsFloatCount = ( 17 + 17 + 1 ) * 4;

Scene::ModelEntry::ModelEntry( std::shared_ptr<Mesh> mesh, FXMMATRIX transform, std::set< int > subsets )
  : mesh( mesh )
  , subsets( subsets )
{
  XMStoreFloat4x4( &this->nodeTransform, transform );
  randomValues.x = Random();
  randomValues.y = Random();
  randomValues.z = Random();
  randomValues.w = Random();
}

void Scene::ModelEntry::SetTransform( FXMMATRIX transform )
{
  XMStoreFloat4x4( &this->nodeTransform, transform );
}

BoundingBox Scene::ModelEntry::CalcBoundingBox() const
{
  BoundingBox transformedBox;
  mesh->CalcBoundingBox().Transform( transformedBox, XMLoadFloat4x4( &nodeTransform ) );
  return transformedBox;
}

BoundingOrientedBox Scene::ModelEntry::CalcBoundingOrientedBox() const
{
  BoundingOrientedBox transformedBox;
  BoundingOrientedBox::CreateFromBoundingBox( transformedBox, mesh->CalcBoundingBox() );
  transformedBox.Transform( transformedBox, XMLoadFloat4x4( &nodeTransform ) );
  return transformedBox;
}

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::SetUp( CommandList& commandList, Window& window )
{
  auto& renderManager  = RenderManager::GetInstance();
  auto& device         = renderManager.GetDevice();

  auto toneMappingFile       = ReadFileToMemory( L"Content/Shaders/ToneMapping.cso" );
  auto combineLightingFile   = ReadFileToMemory( L"Content/Shaders/CombineLighting.cso" );
  auto downsampleFile        = ReadFileToMemory( L"Content/Shaders/Downsample.cso" );
  auto blurFile              = ReadFileToMemory( L"Content/Shaders/Blur.cso" );
  auto traceGIProbeFile      = ReadFileToMemory( L"Content/Shaders/TraceGIProbe.cso" );
  auto adaptExposureFile     = ReadFileToMemory( L"Content/Shaders/AdaptExposure.cso" );
  auto specBRDFLUTFile       = ReadFileToMemory( L"Content/Shaders/SpecBRDFLUT.cso" );
  auto processReflectionFile = ReadFileToMemory( L"Content/Shaders/ProcessReflection.cso" );
  auto extractBloomFile      = ReadFileToMemory( L"Content/Shaders/ExtractBloom.cso" );
  auto blurBloomFile         = ReadFileToMemory( L"Content/Shaders/BlurBloom.cso" );

  frameConstantBuffer     = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( FrameParamsCB ), sizeof( FrameParamsCB ), L"FrameParams" );
  prevFrameConstantBuffer = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( FrameParamsCB ), sizeof( FrameParamsCB ), L"PrevFrameParams" );
  lightingConstantBuffer  = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( LightingEnvironmentParamsCB ), sizeof( LightingEnvironmentParamsCB ), L"LightingParams" );
  gaussianBuffer          = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( float ) * BlurParamsFloatCount, sizeof( float ) * BlurParamsFloatCount, L"GaussianBuffer" );
  exposureBuffer          = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, true,  sizeof( float ), sizeof( float ), L"ExposureBuffer" );
  allMeshParamsBuffer     = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( MeshParamsCB ) * MaxInstanceCount, sizeof( MeshParamsCB ), L"AllMeshParams" );
  toneMappingShader       = device.CreateComputeShader( toneMappingFile.data(), int( toneMappingFile.size() ), L"ToneMapping" );
  combineLightingShader   = device.CreateComputeShader( combineLightingFile.data(), int( combineLightingFile.size() ), L"CombineLighting" );
  downsampleShader        = device.CreateComputeShader( downsampleFile.data(), int( downsampleFile.size() ), L"Downsample" );
  blurShader              = device.CreateComputeShader( blurFile.data(), int( blurFile.size() ), L"Blur" );
  traceGIProbeShader      = device.CreateComputeShader( traceGIProbeFile.data(), int( traceGIProbeFile.size() ), L"TraceGIProbe" );
  adaptExposureShader     = device.CreateComputeShader( adaptExposureFile.data(), int( adaptExposureFile.size() ), L"AdaptExposure" );
  specBRDFLUTShader       = device.CreateComputeShader( specBRDFLUTFile.data(), int( specBRDFLUTFile.size() ), L"SpecBRDFLUT" );
  processReflectionShader = device.CreateComputeShader( processReflectionFile.data(), int( processReflectionFile.size() ), L"ProcessReflection" );
  extractBloomShader      = device.CreateComputeShader( extractBloomFile.data(), int( extractBloomFile.size() ), L"ExtractBloomFile" );
  blurBloomShader         = device.CreateComputeShader( blurBloomFile.data(), int( blurBloomFile.size() ), L"BlurBloomFile" );
  
  auto gaussianBufferDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ConstantBufferView, GaussianBufferSlot, *gaussianBuffer, sizeof( float ) * BlurParamsFloatCount );
  gaussianBuffer->AttachResourceDescriptor( ResourceDescriptorType::ConstantBufferView, std::move( gaussianBufferDesc ) );

  auto exposureBufferUAVDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::UnorderedAccessView, ExposureBufferUAVSlot, *exposureBuffer, sizeof( float ) );
  exposureBuffer->AttachResourceDescriptor( ResourceDescriptorType::UnorderedAccessView, std::move( exposureBufferUAVDesc ) );

  auto allMeshParamsDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, AllMeshParamsSlot, *allMeshParamsBuffer, sizeof( MeshParamsCB ) );
  allMeshParamsBuffer->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( allMeshParamsDesc ) );

  specBRDFLUTTexture = device.Create2DTexture( commandList, SpecBRDFLUTSize, SpecBRDFLUTSize, nullptr, 0, PixelFormat::RG1616F, false, SpecBRDFLUTSlot, SpecBRDFLUTUAVSlot, false, L"SpecBRDFLUT" );

  commandList.ChangeResourceState( *specBRDFLUTTexture, ResourceStateBits::UnorderedAccess );
  commandList.SetComputeShader( *specBRDFLUTShader );
  commandList.SetComputeResource( 0, *specBRDFLUTTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
  commandList.Dispatch( SpecBRDFLUTSize / 32, SpecBRDFLUTSize / 32, 1 );
  commandList.AddUAVBarrier( *specBRDFLUTTexture );
  commandList.ChangeResourceState( *specBRDFLUTTexture, ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );

  RecreateWindowSizeDependantResources( commandList, window );
}

void Scene::SetSky( CommandList& commandList, const GUID& guid )
{
  auto iter = gameDefinition.GetSkies().find( guid );
  if ( iter != gameDefinition.GetSkies().end() )
  {
    if ( skybox )
      skybox->Dispose( commandList );

    auto& sky   = iter->second;
    skyMaterial = RenderManager::GetInstance().GetAllMaterialIndex( sky.material );
    skybox      = Mesh::CreateSkybox( commandList, skyMaterial );
    SetDirectionalLight( XMVector3Normalize( XMLoadFloat3( &sky.lightDirection ) ), 1000, 40, XMLoadFloat3( &sky.lightColor ), sky.lightIntensity, true );
  }
  else
    skybox.reset();
}

uint32_t Scene::AddMesh( CommandList& commandList, const wchar_t* filePath, bool forceReload, FXMMATRIX transform, std::set< int > subsets, MaterialTranslator materialTranslator )
{
  auto meshIter = loadedMeshes.find( filePath );
  if ( meshIter != loadedMeshes.end() && forceReload )
  {
    meshIter->second->Dispose( commandList );
    loadedMeshes.erase( meshIter );
    meshIter = loadedMeshes.end();
  }
  if ( meshIter == loadedMeshes.end() )
  {
    std::shared_ptr< Mesh > mesh = Mesh::CreateFromMeshFile( commandList, filePath, materialTranslator );
    meshIter = loadedMeshes.emplace( std::wstring( filePath ), mesh ).first;
  }

  auto id = entryCounter++;
  modelEntries[ id ] = ModelEntry( meshIter->second, transform, std::move( subsets ) );
  rtState = RTState::ElementsModified;
  return id;
}

uint32_t Scene::AddMesh( std::shared_ptr< Mesh > mesh, FXMMATRIX transform, std::set< int > subsets )
{
  auto id = entryCounter++;
  modelEntries[ id ] = ModelEntry( mesh, transform, std::move( subsets ) );
  rtState = RTState::ElementsModified;
  return id;
}

Mesh* Scene::GetMesh( uint32_t id )
{
  auto iter = modelEntries.find( id );
  if ( iter == modelEntries.end() )
    return nullptr;

  return iter->second.mesh.get();
}

void Scene::SetVertexBufferForMesh( uint32_t id, Resource* vertexBuffer )
{
  auto iter = modelEntries.find( id );
  if ( iter == modelEntries.end() )
    return;

  iter->second.vertexBufferOverride = vertexBuffer;
}

void Scene::SetRayTracingForMesh( uint32_t id, int subset, RTBottomLevelAccelerator* rtAccelerator )
{
  auto iter = modelEntries.find( id );
  if ( iter == modelEntries.end() )
    return;

  auto& accel = iter->second.rtAccelerators[ subset ];

  if ( !!accel != !!rtAccelerator )
    rtState = RTState::ElementsModified;
  else if ( rtState != RTState::ElementsModified )
    rtState = RTState::TrianglesModified;

  accel = rtAccelerator;
}

void Scene::RemoveMesh( uint32_t id )
{
  // TODO: unload model after last reference
  modelEntries.erase( id );
  rtState = RTState::ElementsModified;
}

void Scene::MoveMesh( uint32_t id, FXMMATRIX worldTransform )
{
  auto iter = modelEntries.find( id );
  assert( iter != modelEntries.end() );

  iter->second.SetTransform( worldTransform );

  if ( rtState != RTState::ElementsModified )
    rtState = RTState::TrianglesModified;
}

void Scene::SetDirectionalLight( FXMVECTOR direction, float distance, float radius, FXMVECTOR color, float intensity, bool castShadow )
{
  directionalLight.type             = LightTypeCB::Directional;
  directionalLight.sourceRadius     = radius;
  directionalLight.attenuationStart = distance;
  directionalLight.castShadow       = castShadow;
  XMStoreFloat4( &directionalLight.direction, direction );
  XMStoreFloat4( &directionalLight.color, color * intensity );
}

void Scene::SetCamera( Camera& camera )
{
  // TODO: some preceise clip plane calculation
  float nearZ = 0.1f;
  float farZ  = 200.0f;

  camera.SetDepthRange( nearZ, farZ );

  XMStoreFloat4x4( &frameParams.vpTransform,      camera.GetViewProjectionTransform() );
  XMStoreFloat4x4( &frameParams.viewTransform,    camera.GetViewTransform() );
  XMStoreFloat4x4( &frameParams.projTransform,    camera.GetProjectionTransform() );
  XMStoreFloat4x4( &frameParams.invProjTransform, XMMatrixInverse( nullptr, camera.GetProjectionTransform() ) );
  XMStoreFloat4  ( &frameParams.cameraPosition,   camera.GetPosition() );
  XMStoreFloat4  ( &frameParams.cameraDirection,  camera.GetDirection() );
  frameParams.screenSize.x = float( hdrTexture->GetTextureWidth() );
  frameParams.screenSize.y = float( hdrTexture->GetTextureHeight() );
  frameParams.screenSize.z = 1.0f / frameParams.screenSize.x;
  frameParams.screenSize.w = 1.0f / frameParams.screenSize.y;
}

float GaussianDistribution( float x, float m, float s )
{
  static const float inv_sqrt_2pi = 0.3989422804014327f;
  float a = ( x - m ) / s;

  return inv_sqrt_2pi / s * std::exp( -0.5f * a * a );
}

static void CalcGaussianblurParams( float* offsets, float* weights, float td )
{
  float totalWeight = 0.0f;
  size_t index = 0;
  for ( int x = -8; x <= 8; ++x )
  {
      offsets[ index * 4 ] = float( x ) * td;
      weights[ index * 4 ] = GaussianDistribution( float( x ), 0, 4.0f );
      totalWeight += weights[ index * 4 ];

      ++index;
  }

  for ( size_t i = 0; i < index; ++i )
    weights[ i * 4 ] = weights[ i * 4 + 1 ] = weights[ i * 4 + 2 ] = weights[ i * 4 + 3 ] = weights[ i * 4 ] / totalWeight;
}

std::pair< Resource&, Resource& > Scene::Render( CommandList& commandList, const EditorInfo& editorInfo, float dt )
{
  sceneTime += dt;

  auto& renderManager   = RenderManager::GetInstance();
  auto  vTransform      = XMLoadFloat4x4( &frameParams.viewTransform );
  auto  pTransform      = XMLoadFloat4x4( &frameParams.projTransform );
  auto  vpTransform     = XMLoadFloat4x4( &frameParams.vpTransform );
  auto  prevVTransform  = XMLoadFloat4x4( &prevFrameParams.viewTransform );
  auto  prevVPTransform = XMLoadFloat4x4( &prevFrameParams.vpTransform );
  auto  cameraPosition  = XMLoadFloat4( &prevFrameParams.cameraPosition );
  auto  prevTargetIndex = currentTargetIndex;
  currentTargetIndex    = 1 - currentTargetIndex;
  currentGISource       = 1 - currentGISource;

  frameParams.frameIndex     = ( frameParams.frameIndex + 1 ) % 10000;
  frameParams.depthIndex     = currentTargetIndex == 0 ? DepthSlot0 : DepthSlot1;
  frameParams.prevDepthIndex = currentTargetIndex == 0 ? DepthSlot1 : DepthSlot0;
  frameParams.aoIndex        = currentTargetIndex == 0 ? DirectLighting1Slot : DirectLighting2Slot;
  frameParams.prevAOIndex    = currentTargetIndex == 0 ? DirectLighting2Slot : DirectLighting1Slot;

  frameParams.giSourceIndex = ( currentGISource == 0 ) ? GITexture1SlotHLSL : GITexture2SlotHLSL;
  
  XMFLOAT3 worldAnchor( sceneAABB.Center.x - sceneAABB.Extents.x, sceneAABB.Center.y - sceneAABB.Extents.y, sceneAABB.Center.z - sceneAABB.Extents.z );

  frameParams.invWorldSize.x   = 1.0f / ( 2 * sceneAABB.Extents.x );
  frameParams.invWorldSize.y   = 1.0f / ( 2 * sceneAABB.Extents.y );
  frameParams.invWorldSize.z   = 1.0f / ( 2 * sceneAABB.Extents.z );
  frameParams.worldMinCorner.x = worldAnchor.x;
  frameParams.worldMinCorner.y = worldAnchor.y;
  frameParams.worldMinCorner.z = worldAnchor.z;

  frameParams.globalTime = sceneTime;

  frameParams.frameDebugMode = editorInfo.frameDebugMode;

  auto& depthTexture     = depthTextures[ currentTargetIndex ];
  auto& prevDepthTexture = depthTextures[ prevTargetIndex ];

  auto& directLightingTexture     = directLightingTextures[ currentTargetIndex ];
  auto& prevDirectLightingTexture = directLightingTextures[ prevTargetIndex ];

  auto renderWidth  = hdrTexture->GetTextureWidth();
  auto renderHeight = hdrTexture->GetTextureHeight();

  BoundingFrustum cameraFrustum;
  BoundingFrustum::CreateFromMatrix( cameraFrustum, pTransform );
  cameraFrustum.Transform( cameraFrustum, XMMatrixInverse( nullptr, vTransform ) );

  struct ModelEntryWithIndex
  {
    ModelEntryWithIndex( ModelEntry& entry, int index ) : entry( entry ), index( index ) {}
    ModelEntry& entry;
    int         index = 0;
  };
  std::map< uint32_t, ModelEntryWithIndex > visibleEntries;

  allMeshParams.clear();

  skybox->PrepareMeshParams( allMeshParams
                           , XMMatrixIdentity()
                           , XMMatrixIdentity()
                           , XMMatrixIdentity()
                           , XMMatrixIdentity()
                           , XMMatrixIdentity()
                           , XMMatrixIdentity()
                           , { 0 }
                           , XMFLOAT4()
                           , 0 );

  bool hasSkinned = false;
  for ( auto& entry : modelEntries )
  {
    auto bb = entry.second.CalcBoundingOrientedBox();
    if ( cameraFrustum.Contains( bb ) != ContainmentType::DISJOINT )
    {
      auto modelTransform     = XMLoadFloat4x4( &entry.second.nodeTransform );
      auto prevModelTransform = XMLoadFloat4x4( &entry.second.prevNodeTransform );
      auto editorAddition     = editorInfo.selection.count( entry.first ) ? 0.50f : 0.0f;
      auto index              = int( allMeshParams.size() );
      entry.second.mesh->PrepareMeshParams( allMeshParams
                                          , modelTransform
                                          , vTransform
                                          , vpTransform
                                          , prevModelTransform
                                          , prevVPTransform
                                          , prevVTransform
                                          , entry.second.subsets
                                          , entry.second.randomValues
                                          , editorAddition );

      visibleEntries.emplace( entry.first, ModelEntryWithIndex( entry.second, index ) );
    }
    else
      continue;

    if ( entry.second.mesh->IsSkin() )
      hasSkinned = true;
  }

  assert( allMeshParams.size() < MaxInstanceCount );

  auto allMeshParamsSize = int( sizeof( MeshParamsCB ) * allMeshParams.size() );
  auto uploadAllMeshParamsBuffer = RenderManager::GetInstance().GetUploadConstantBufferForResource( *allMeshParamsBuffer );
  commandList.UploadBufferResource( std::move( uploadAllMeshParamsBuffer ), *allMeshParamsBuffer, allMeshParams.data(), allMeshParamsSize );
  commandList.ChangeResourceState( *allMeshParamsBuffer, ResourceStateBits::NonPixelShaderInput | ResourceStateBits::PixelShaderInput );

  if ( giProbeInstancesDirty )
  {
    RefreshGIProbeInstances( commandList );
    giProbeInstancesDirty = false;
  }

  auto renderMesh = [&]( uint32_t id, ModelEntry& entry, int baseIndex, AlphaModeCB alphaMode )
  {
    auto modelTransform     = XMLoadFloat4x4( &entry.nodeTransform );
    auto prevModelTransform = XMLoadFloat4x4( &entry.prevNodeTransform );
    auto editorAddition     = editorInfo.selection.count( id ) ? 0.40f : 0.0f;
    entry.mesh->Render( renderManager.GetDevice()
                      , commandList
                      , baseIndex
                      , entry.subsets
                      , alphaMode
                      , entry.vertexBufferOverride );
  };

  auto setUpDepthPass = [&]( PipelinePresets preset )
  {
    commandList.SetPipelineState( renderManager.GetPipelinePreset( preset ) );
    commandList.SetConstantBuffer( 1, *frameConstantBuffer );
    commandList.SetTextureHeap( 3, renderManager.GetShaderResourceHeap(), 0 );

    renderManager.BindAllMaterials( commandList, 2 );
  };

  auto setUpOpaquePass = [&]( PipelinePresets preset )
  {
    commandList.SetPipelineState( renderManager.GetPipelinePreset( preset ) );
    commandList.SetConstantBuffer( 1, *frameConstantBuffer );
    commandList.SetConstantBuffer( 2, *prevFrameConstantBuffer );
    commandList.SetConstantBuffer( 3, *lightingConstantBuffer );
    commandList.SetRayTracingScene( 5, *rtDescriptor );
    commandList.SetTextureHeap( 6, renderManager.GetShaderResourceHeap(), 0 );

    renderManager.BindAllMaterials( commandList, 4 );
  };

  {
    commandList.BeginEvent( 0, L"Scene::Render(Frame setup)" );

    auto ubp = renderManager.GetUploadConstantBufferForResource( *prevFrameConstantBuffer );
    commandList.UploadBufferResource( std::move( ubp ), *prevFrameConstantBuffer, &prevFrameParams, sizeof( FrameParamsCB ) );
    auto ubc = renderManager.GetUploadConstantBufferForResource( *frameConstantBuffer );
    commandList.UploadBufferResource( std::move( ubc ), *frameConstantBuffer, &frameParams, sizeof( FrameParamsCB ) );

    commandList.EndEvent();
  }

  if ( giProbeTextures[ 0 ] && giProbeTextures[ 1 ] )
  {
    commandList.BeginEvent( 0, L"Scene::Render(TraceGIProbes)" );

    commandList.CopyResource( *giProbeTextures[ 1 - currentGISource ], *giProbeTextures[ currentGISource ] );

    commandList.ChangeResourceState( *giProbeTextures[ 1 - currentGISource ], ResourceStateBits::NonPixelShaderInput | ResourceStateBits::PixelShaderInput );
    commandList.ChangeResourceState( *giProbeTextures[ currentGISource ], ResourceStateBits::UnorderedAccess );

    commandList.SetComputeShader( *traceGIProbeShader );

    commandList.SetComputeConstantValues( 0, giProbeUpdateNext );
    commandList.SetComputeRayTracingScene( 1, *rtDescriptor );
    commandList.SetComputeConstantBuffer( 2, *lightingConstantBuffer );
    commandList.SetComputeConstantBuffer( 3, *frameConstantBuffer );
    commandList.SetComputeConstantBuffer( 4, *prevFrameConstantBuffer );
    renderManager.BindAllMaterialsToCompute( commandList, 5 );
    commandList.SetComputeResource( 6, *giProbeTextures[ currentGISource ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.SetComputeTextureHeap( 7, renderManager.GetShaderResourceHeap(), 0 );

    commandList.Dispatch( giProbeUpdatePerFrame, 1, 1 );
    commandList.AddUAVBarrier( *giProbeTextures[ currentGISource ] );
    commandList.ChangeResourceState( *giProbeTextures[ currentGISource ], ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );

    giProbeUpdateNext = ( giProbeUpdateNext + giProbeUpdatePerFrame ) % ( frameParams.giProbeCount.x * frameParams.giProbeCount.y * frameParams.giProbeCount.z );

    commandList.EndEvent();
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(Back buffer setup)" );

    commandList.ChangeResourceState( *sdrTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *directLightingTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *indirectLightingTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *reflectionTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *depthTexture, ResourceStateBits::DepthWrite );
    commandList.ChangeResourceState( *prevDepthTexture, ResourceStateBits::DepthRead | ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );
    commandList.Clear( *depthTexture, 1 );
    commandList.Clear( *reflectionTexture, Color( 0, 0, 0, 0 ) );

    commandList.SetViewport( 0, 0, renderWidth, renderHeight );
    commandList.SetScissor ( 0, 0, renderWidth, renderHeight );

    commandList.EndEvent();
  }

  if ( !visibleEntries.empty() )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Depth pass opaque)" );

    commandList.SetRenderTarget( {}, depthTexture.get() );

    setUpDepthPass( PipelinePresets::Depth );

    for ( auto& entry : visibleEntries )
      if ( !entry.second.entry.mesh->IsSkin() )
        renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::Opaque );

    if ( hasSkinned )
    {
      setUpDepthPass( PipelinePresets::DepthWithHistory );

      for ( auto& entry : visibleEntries )
        if ( entry.second.entry.mesh->IsSkin() )
          renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::Opaque );

    }

    commandList.EndEvent();
  }

  if ( !visibleEntries.empty() )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Depth pass one bit alpha)" );

    commandList.SetRenderTarget( *sdrTexture, depthTexture.get() );

    setUpDepthPass( PipelinePresets::DepthOneBitAlpha );

    for ( auto& entry : visibleEntries )
      if ( !entry.second.entry.mesh->IsSkin() )
        renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::OneBitAlpha );

    if ( hasSkinned )
    {
      setUpDepthPass( PipelinePresets::DepthOneBitAlphaWithHistory );

      for ( auto& entry : visibleEntries )
        if ( entry.second.entry.mesh->IsSkin() )
          renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::OneBitAlpha );
    }

    commandList.EndEvent();
  }

  if ( !visibleEntries.empty() )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Production pass opaque)" );

    commandList.ChangeResourceState( *directLightingTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *prevDirectLightingTexture, ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );

    commandList.SetRenderTarget( { directLightingTexture.get()
                                 , indirectLightingTexture.get()
                                 , reflectionTexture.get() }
                               , depthTexture.get() );

    setUpOpaquePass( PipelinePresets::Mesh );

    for ( auto& entry : visibleEntries )
      if ( !entry.second.entry.mesh->IsSkin() )
        renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::Opaque );

    if ( hasSkinned )
    {
      setUpOpaquePass( PipelinePresets::MeshWithHistory );

      for ( auto& entry : visibleEntries )
        if ( entry.second.entry.mesh->IsSkin() )
          renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::Opaque );
    }

    commandList.EndEvent();
  }

  if ( !visibleEntries.empty() )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Production pass one bit alpha)" );

    setUpOpaquePass( PipelinePresets::MeshOneBitAlpha );

    for ( auto& entry : visibleEntries )
      if ( !entry.second.entry.mesh->IsSkin() )
        renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::OneBitAlpha );

    if ( hasSkinned )
    {
      setUpOpaquePass( PipelinePresets::MeshOneBitAlphaWithHistory );

      for ( auto& entry : visibleEntries )
        if ( entry.second.entry.mesh->IsSkin() )
          renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::OneBitAlpha );
    }

    commandList.EndEvent();
  }

  int finalReflectionProcIndex = -1;

  {
    commandList.BeginEvent( 0, L"Scene::Render(Processing reflection)" );

    commandList.AddUAVBarrier( *reflectionTexture );

    commandList.ChangeResourceState( *reflectionTexture, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *reflectionProcTextures[ 0 ], ResourceStateBits::UnorderedAccess );

    commandList.SetComputeShader( *downsampleShader );
    commandList.SetComputeResource( 0, *reflectionTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 1, *reflectionProcTextures[ 0 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( reflectionProcTextures[ 0 ]->GetTextureWidth () + DownsamplingKernelWidth  - 1 ) / DownsamplingKernelWidth
                        , ( reflectionProcTextures[ 0 ]->GetTextureHeight() + DownsamplingKernelHeight - 1 ) / DownsamplingKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *reflectionProcTextures[ 0 ] );

    //=================================================================================================

    commandList.ChangeResourceState( *depthTexture, ResourceStateBits::NonPixelShaderInput );

    commandList.SetComputeShader( *processReflectionShader );
    commandList.SetComputeResource( 1, *depthTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );

    int sourceIx = 0;

    struct
    {
      XMFLOAT4X4 invProj;
      int        texWidth;
      int        texHeight;
      float      invTexWidth;
      float      invTexHeight;
      XMFLOAT2   taps[ ReflectionProcessTaps ];
    } params;

    assert( atoi( ReflectionProcessDataSizeStr ) == sizeof( params ) / 4 );

    params.texWidth     = reflectionProcTextures[ 0 ]->GetTextureWidth();
    params.texHeight    = reflectionProcTextures[ 0 ]->GetTextureHeight();
    params.invTexWidth  = 1.0f / reflectionProcTextures[ 0 ]->GetTextureWidth();
    params.invTexHeight = 1.0f / reflectionProcTextures[ 0 ]->GetTextureHeight();
    params.invProj      = frameParams.invProjTransform;

    const auto iterations = 12;
    const auto stepSize   = 2.0f * PI / ReflectionProcessTaps;
    const auto maxD       = 0.1f;
    const auto stepD      = maxD / iterations;

    for ( int iter = 0; iter < iterations; ++iter )
    {
      commandList.ChangeResourceState( *reflectionProcTextures[ sourceIx ], ResourceStateBits::NonPixelShaderInput );
      commandList.ChangeResourceState( *reflectionProcTextures[ 1 - sourceIx ], ResourceStateBits::UnorderedAccess );

      commandList.SetComputeResource( 2, *reflectionProcTextures[ sourceIx ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
      commandList.SetComputeResource( 3, *reflectionProcTextures[ 1 - sourceIx ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );

      auto angle = iter * stepSize / iterations;
      for ( auto cit = 0; cit < ReflectionProcessTaps; ++cit )
      {
        params.taps[ cit ].x = sin( angle ) * ( iter + 1 ) * stepD;
        params.taps[ cit ].y = cos( angle ) * ( iter + 1 ) * stepD;
        angle += stepSize;
      }

      commandList.SetComputeConstantValues( 0, params );
      commandList.Dispatch( ( reflectionProcTextures[ 0 ]->GetTextureWidth () + ReflectionProcessKernelWidth  - 1 ) / ReflectionProcessKernelWidth
                          , ( reflectionProcTextures[ 0 ]->GetTextureHeight() + ReflectionProcessKernelHeight - 1 ) / ReflectionProcessKernelHeight
                          , 1 );

      commandList.AddUAVBarrier( *reflectionProcTextures[ 1 - sourceIx ] );

      sourceIx = 1 - sourceIx;
    }

    finalReflectionProcIndex = sourceIx;

    commandList.ChangeResourceState( *depthTexture, ResourceStateBits::DepthWrite );

    commandList.EndEvent();
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(Combine lighting)" );

    commandList.ChangeResourceState( *directLightingTexture, ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *indirectLightingTexture, ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *reflectionTexture, ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *reflectionProcTextures[ finalReflectionProcIndex ], ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *hdrTexture, ResourceStateBits::UnorderedAccess );

    struct
    {
      float            invTexWidth;
      float            invTexHeight;
      FrameDebugModeCB debugMode;
    } params;

    params.invTexWidth  = 1.0f / hdrTexture->GetTextureWidth();
    params.invTexHeight = 1.0f / hdrTexture->GetTextureHeight();
    params.debugMode    = frameParams.frameDebugMode;

    commandList.SetComputeShader( *combineLightingShader );
    commandList.SetComputeConstantValues( 0, params );
    commandList.SetComputeResource( 1, *directLightingTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 2, *indirectLightingTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 3, *reflectionTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 4, *reflectionProcTextures[ finalReflectionProcIndex ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 5, *hdrTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( hdrTexture->GetTextureWidth () + LightCombinerKernelWidth - 1  ) / LightCombinerKernelWidth
                        , ( hdrTexture->GetTextureHeight() + LightCombinerKernelHeight - 1 ) / LightCombinerKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *hdrTexture );

    commandList.EndEvent();
  }

  if ( drawGIProbes && giProbeTextures[ currentGISource ] )
  {
    commandList.BeginEvent( 0, L"Scene::Render(GI probes)" );

    commandList.ChangeResourceState( *hdrTexture, ResourceStateBits::RenderTarget );
    commandList.SetRenderTarget( *hdrTexture, depthTexture.get() );

    commandList.SetPipelineState( renderManager.GetPipelinePreset( PipelinePresets::GIProbes ) );
    commandList.SetConstantValues( 0, frameParams.vpTransform );
    commandList.SetConstantBuffer( 1, *frameConstantBuffer );
    commandList.SetTextureHeap( 2, renderManager.GetShaderResourceHeap(), 0 );

    commandList.SetVertexBuffer( *giProbeVB );
    commandList.SetIndexBuffer( *giProbeIB );

    commandList.SetVariableRateShading( VRSBlock::_1x1 );

    commandList.SetPrimitiveType( PrimitiveType::TriangleList );
    commandList.DrawIndexed( giProbeIndexCount, frameParams.giProbeCount.x * frameParams.giProbeCount.y * frameParams.giProbeCount.z );

    commandList.EndEvent();
  }

  if ( skybox )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Skybox)" );

    commandList.ChangeResourceState( *hdrTexture, ResourceStateBits::RenderTarget );
    commandList.SetRenderTarget( *hdrTexture, depthTexture.get() );

    commandList.SetPipelineState( renderManager.GetPipelinePreset( PipelinePresets::Skybox ) );
    commandList.SetConstantBuffer( 1, *frameConstantBuffer );
    commandList.SetTextureHeap( 3, renderManager.GetShaderResourceHeap(), 0 );

    renderManager.BindAllMaterials( commandList, 2 );

    skybox->Render( renderManager.GetDevice(), commandList, 0, { 0 }, AlphaModeCB::Opaque, nullptr, PrimitiveType::TriangleStrip );

    commandList.EndEvent();
  }

  if ( !visibleEntries.empty() )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Translucent pass)" );

    auto cameraPosition = XMLoadFloat4( &frameParams.cameraPosition );

    std::multimap< float, std::pair< uint32_t, ModelEntryWithIndex* > > sortedModels;
    for ( auto& entry : visibleEntries )
    {
      if ( entry.second.entry.mesh->HasTranslucent() )
      {
        auto entityCenter   = entry.second.entry.mesh->CalcCenter();
        auto worldTransform = XMLoadFloat4x4( &entry.second.entry.nodeTransform );
        auto worldPosition  = XMVector4Transform( entityCenter, worldTransform );
        auto distance       = XMVectorGetX( XMVector3Length( XMVectorSubtract( cameraPosition, worldPosition ) ) );
        sortedModels.insert( { distance, { entry.first, &entry.second } } );
      }
    }

    int sideSetup = 0;
    for ( auto iter = sortedModels.rbegin(); iter != sortedModels.rend(); ++iter )
    {
      int desiredSides = iter->second.second->entry.mesh->GetTranslucentSidesMode();
      if ( sideSetup != desiredSides )
      {
        commandList.BeginEvent( 0, desiredSides == 1 ? L"Switching to one sided mode" : L"Switching to two sided mode" );

        setUpOpaquePass( desiredSides == 1 ? PipelinePresets::MeshTranslucent : PipelinePresets::MeshTranslucentTwoSided );
        sideSetup = desiredSides;

        commandList.EndEvent();
      }

      renderMesh( iter->second.first, iter->second.second->entry, iter->second.second->index, AlphaModeCB::Translucent );
    }

    commandList.EndEvent();
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(Adapt exposure)" );

    commandList.SetVariableRateShading( VRSBlock::_1x1 );
    commandList.GenerateMipmaps( *hdrTexture );

    struct
    {
      float manualExposure;
      float timeElapsed;
      float speed;
    } params;

    params.manualExposure = exposure;
    params.timeElapsed    = dt;
    params.speed          = 50;

    commandList.ChangeResourceState( *exposureBuffer, ResourceStateBits::UnorderedAccess );

    commandList.SetComputeShader( *adaptExposureShader );
    commandList.SetComputeConstantValues( 0, params );
    commandList.SetComputeResource( 1, *hdrTextureLowLevel );
    commandList.SetComputeResource( 2, *exposureBuffer->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( 1, 1, 1 );

    commandList.AddUAVBarrier( *exposureBuffer );

    commandList.EndEvent();
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(Extract bloom)" );

    commandList.ChangeResourceState( *hdrTexture, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *exposureBuffer, ResourceStateBits::VertexOrConstantBuffer );
    commandList.ChangeResourceState( *bloomTexture, ResourceStateBits::UnorderedAccess );

    struct
    {
      float invOutputWidth;
      float invOutputHeight;
      float bloomThreshold;
    } params;

    params.invOutputWidth  = 1.0f / bloomTexture->GetTextureWidth();
    params.invOutputHeight = 1.0f / bloomTexture->GetTextureHeight();
    params.bloomThreshold  = bloomThreshold;

    commandList.SetComputeShader( *extractBloomShader );
    commandList.SetComputeConstantValues( 0, params );
    commandList.SetComputeResource( 1, *hdrTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeConstantBuffer( 2, *exposureBuffer );
    commandList.SetComputeResource( 3, *bloomTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( bloomTexture->GetTextureWidth () + ExtractBloomKernelWidth - 1  ) / ExtractBloomKernelWidth
                        , ( bloomTexture->GetTextureHeight() + ExtractBloomKernelHeight - 1 ) / ExtractBloomKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *bloomTexture );

    commandList.EndEvent();
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(Blur bloom)" );

    commandList.ChangeResourceState( *bloomTexture, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *bloomBlurredTexture, ResourceStateBits::UnorderedAccess );

    struct
    {
      float invOutputWidth;
      float invOutputHeight;
    } params;

    params.invOutputWidth  = 1.0f / bloomTexture->GetTextureWidth();
    params.invOutputHeight = 1.0f / bloomTexture->GetTextureHeight();

    commandList.SetComputeShader( *blurBloomShader );
    commandList.SetComputeConstantValues( 0, params );
    commandList.SetComputeResource( 1, *bloomTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 2, *bloomBlurredTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( bloomBlurredTexture->GetTextureWidth () + BlurBloomKernelWidth - 1  ) / BlurBloomKernelWidth
                        , ( bloomBlurredTexture->GetTextureHeight() + BlurBloomKernelHeight - 1 ) / BlurBloomKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *bloomBlurredTexture );

    commandList.EndEvent();
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(ToneMapping)" );

    commandList.ChangeResourceState( *exposureBuffer, ResourceStateBits::VertexOrConstantBuffer );
    commandList.ChangeResourceState( *hdrTexture, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *bloomBlurredTexture, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *sdrTexture, ResourceStateBits::UnorderedAccess );

    struct
    {
      float invTexWidth;
      float invTexHeight;
      float bloomStrength;
    } params;

    params.invTexWidth   = 1.0f / sdrTexture->GetTextureWidth();
    params.invTexHeight  = 1.0f / sdrTexture->GetTextureHeight();
    params.bloomStrength = bloomStrength;

    commandList.SetComputeShader( *toneMappingShader );
    commandList.SetComputeResource( 0, *hdrTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeConstantBuffer( 1, *exposureBuffer );
    commandList.SetComputeConstantValues( 2, params );
    commandList.SetComputeResource( 3, *bloomBlurredTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 4, *sdrTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( hdrTexture->GetTextureWidth () + ToneMappingKernelWidth  - 1 ) / ToneMappingKernelWidth
                        , ( hdrTexture->GetTextureHeight() + ToneMappingKernelHeight - 1 ) / ToneMappingKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *sdrTexture );

    commandList.EndEvent();
  }

  if ( editorInfo.gizmoType != GizmoType::None )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Gizmo)" );

    if ( !gizmo || gizmo->GetType() != editorInfo.gizmoType )
    {
      if ( gizmo )
        gizmo->TearDown( commandList );
      gizmo = std::make_unique< Gizmo >( commandList, editorInfo.gizmoType );
    }

    commandList.ChangeResourceState( *sdrTexture, ResourceStateBits::RenderTarget );
    commandList.SetRenderTarget( *sdrTexture, depthTexture.get() );
    commandList.Clear( *depthTexture, 1 );

    commandList.SetPipelineState( renderManager.GetPipelinePreset( PipelinePresets::Gizmos ) );
    commandList.SetConstantValues( 0, editorInfo.selectedGizmoElement );
    gizmo->Render( commandList, XMMatrixMultiply( XMLoadFloat4x4( &editorInfo.gizmoTransform ), vpTransform ) );

    commandList.EndEvent();
  }


  prevFrameParams = frameParams;

  for ( auto& entry : modelEntries )
    entry.second.prevNodeTransform = entry.second.nodeTransform;

  return std::pair< Resource&, Resource& >( *sdrTexture, *depthTexture );
}

void Scene::SetPostEffectParams( float exposure, float bloomThreshold, float bloomStrength )
{
  this->exposure       = exposure;
  this->bloomThreshold = bloomThreshold;
  this->bloomStrength  = bloomStrength;
}

int Scene::GetTargetWidth() const
{
  return hdrTexture->GetTextureWidth();
}

int Scene::GetTargetHeight() const
{
  return hdrTexture->GetTextureHeight();
}

void Scene::RecreateWindowSizeDependantResources( CommandList& commandList, Window& window )
{
  for ( auto& t : depthTextures ) t.reset();
  for ( auto& t : directLightingTextures ) t.reset();
  for ( auto& t : reflectionProcTextures ) t.reset();
  hdrTexture.reset();
  hdrTextureLowLevel.reset();
  indirectLightingTexture.reset();
  reflectionTexture.reset();
  sdrTexture.reset();
  bloomTexture.reset();
  bloomBlurredTexture.reset();

  auto& device = RenderManager::GetInstance().GetDevice();

  auto width  = window.GetClientWidth();
  auto height = window.GetClientHeight();
  depthTextures[ 0 ]          = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::DepthFormat, false, DepthSlot0,           std::nullopt,           false, L"Depth0" );
  depthTextures[ 1 ]          = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::DepthFormat, false, DepthSlot1,           std::nullopt,           false, L"Depth1" );
  directLightingTextures[ 0 ] = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::HDRFormat,   true,  DirectLighting1Slot,  std::nullopt,           false, L"Lighting1" );
  directLightingTextures[ 1 ] = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::HDRFormat,   true,  DirectLighting2Slot,  std::nullopt,           false, L"Lighting2" );
  hdrTexture                  = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::HDRFormat,   true,  HDRSlot,              HDRUAVSlot,             true,  L"HDR" );
  indirectLightingTexture     = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::HDRFormat,   true,  IndirectLightingSlot, std::nullopt,           false, L"SpecularIBL" );
  reflectionTexture           = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::HDRFormat,   true,  ReflectionSlot,       ReflectionUAVSlot,      false, L"Reflection" );
  sdrTexture                  = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::SDRFormat,   true,  SDRSlot,              SDRUAVSlot,             false, L"SDR" );
  
  reflectionProcTextures[ 0 ] = device.Create2DTexture( commandList, width / 2, height / 2, nullptr, 0, RenderManager::HDRFormat, false, ReflectionProc1Slot, ReflectionProc1UAVSlot, false, L"ReflectionProc1" );
  reflectionProcTextures[ 1 ] = device.Create2DTexture( commandList, width / 2, height / 2, nullptr, 0, RenderManager::HDRFormat, false, ReflectionProc2Slot, ReflectionProc2UAVSlot, false, L"ReflectionProc2" );

  bloomTexture        = device.Create2DTexture( commandList, width / 2, height / 2, nullptr, 0, RenderManager::HDRFormat, true, BloomSlot,        BloomUAVSlot,        false, L"Bloom"        );
  bloomBlurredTexture = device.Create2DTexture( commandList, width / 2, height / 2, nullptr, 0, RenderManager::HDRFormat, true, BloomBlurredSlot, BloomBlurredUAVSlot, false, L"BloomBlurred" );

  hdrTextureLowLevel = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::UnorderedAccessView, HDRLowLevelUAVSlot, *hdrTexture, 0, hdrTexture->GetTextureMipLevels() - 1 );
}

BoundingBox Scene::CalcModelAABB( uint32_t id ) const
{
  auto iter = modelEntries.find( id );
  assert( iter != modelEntries.end() );
  if ( iter == modelEntries.end() )
    return BoundingBox();

  return iter->second.CalcBoundingBox();
}

float Scene::Pick( uint32_t id, FXMVECTOR rayStart, FXMVECTOR rayDir ) const
{
  auto iter = modelEntries.find( id );
  assert( iter != modelEntries.end() );
  if ( iter == modelEntries.end() )
    return -1;

  return iter->second.mesh->Pick( XMLoadFloat4x4( &iter->second.nodeTransform ), rayStart, rayDir );
}

void Scene::ShowGIProbes( bool show )
{
  drawGIProbes = show;
}

Gizmo* Scene::GetGizmo() const
{
  return gizmo.get();
}

void Scene::SetupWetness( CommandList& commandList, const XMINT2& origin, const XMINT2& size, int density, const std::vector< uint8_t >& values )
{
  frameParams.wetnessOriginInvSize.x = float( origin.x );
  frameParams.wetnessOriginInvSize.y = float( origin.y );
  frameParams.wetnessOriginInvSize.z = 1.0f / float( size.x );
  frameParams.wetnessOriginInvSize.w = 1.0f / float( size.y );
  frameParams.wetnessDensity         = float( density );

  if ( !wetnessTexture || wetnessTexture->GetTextureWidth() != size.x * density || wetnessTexture->GetTextureHeight() != size.y * density )
  {
    if ( wetnessTexture )
    {
      wetnessTexture->RemoveResourceDescriptor( ResourceDescriptorType::ShaderResourceView );
      commandList.HoldResource( std::move( wetnessTexture ) );
    }

    auto& device = RenderManager::GetInstance().GetDevice();
    wetnessTexture = device.Create2DTexture( commandList
                                           , size.x * density
                                           , size.y * density
                                           , values.data()
                                           , int( values.size() )
                                           , PixelFormat::R8UN
                                           , false
                                           , WetnessSlot
                                           , std::nullopt
                                           , false
                                           , L"Wetness" );
  }
  else
  {
    auto uploadWetness = RenderManager::GetInstance().GetUploadConstantBufferForResource( *wetnessTexture );
    commandList.UploadTextureResource( std::move( uploadWetness ), *wetnessTexture, values.data(), size.x * density, size.y * density );
  }

  commandList.ChangeResourceState( *wetnessTexture, ResourceStateBits::NonPixelShaderInput | ResourceStateBits::PixelShaderInput );
}

void Scene::RefreshGIProbeInstances( CommandList& commandList )
{
  auto& device = RenderManager::GetInstance().GetDevice();

  commandList.BeginEvent( 2873, L"Scene::RefreshGIProbeInstances" );

  std::vector< BoundingBox > sceneItemsBounding;
  for ( auto& model : modelEntries )
    sceneItemsBounding.emplace_back( std::move( model.second.CalcBoundingBox() ) );

  std::vector< uint8_t > empty( GIProbeWidth * GIProbeHeight * 8 );
  for ( auto& t : empty )
    t = 0;

  frameParams.giProbeOrigin.x = sceneAABB.Center.x - sceneAABB.Extents.x - GIProbeSpacing / 2;
  frameParams.giProbeOrigin.y = sceneAABB.Center.y - sceneAABB.Extents.y + GIProbeSpacing / 2;
  frameParams.giProbeOrigin.z = sceneAABB.Center.z - sceneAABB.Extents.z - GIProbeSpacing / 2;

  frameParams.giProbeCount.x = int( round( ( 2.0 + sceneAABB.Extents.x * 2 ) / GIProbeSpacing ) );
  frameParams.giProbeCount.y = int( round( ( 1.5 + sceneAABB.Extents.y * 2 ) / GIProbeSpacing ) );
  frameParams.giProbeCount.z = int( round( ( 2.0 + sceneAABB.Extents.z * 2 ) / GIProbeSpacing ) );
  frameParams.giProbeCount.w = frameParams.giProbeCount.x * frameParams.giProbeCount.z;

  std::vector< uint64_t > initGI( frameParams.giProbeCount.x * frameParams.giProbeCount.y * frameParams.giProbeCount.z, 0 );

  giProbeTextures[ 0 ] = device.CreateVolumeTexture( commandList
                                                   , frameParams.giProbeCount.x
                                                   , frameParams.giProbeCount.y
                                                   , frameParams.giProbeCount.z
                                                   , initGI.data()
                                                   , int( initGI.size() )
                                                   , PixelFormat::RGBA16161616F
                                                   , GITexture1Slot
                                                   , GITexture1UAVSlot
                                                   , L"GIProbe1" );

  giProbeTextures[ 1 ] = device.CreateVolumeTexture( commandList
                                                   , frameParams.giProbeCount.x
                                                   , frameParams.giProbeCount.y
                                                   , frameParams.giProbeCount.z
                                                   , initGI.data()
                                                   , int( initGI.size() )
                                                   , PixelFormat::RGBA16161616F
                                                   , GITexture2Slot
                                                   , GITexture2UAVSlot
                                                   , L"GIProbe2" );

  currentGISource = 0;

  if ( !giProbeVB )
  {
    std::vector< GIProbeVertexFormat > probeVertices;
    std::vector< uint16_t >            probeIndices;
    Icosahedron( probeVertices, probeIndices );
    SubdivideMesh( probeVertices, probeIndices );
    SubdivideMesh( probeVertices, probeIndices );
    int vbSize = int( sizeof( GIProbeVertexFormat ) * probeVertices.size() );
    int ibSize = int( sizeof( uint16_t ) * probeIndices.size() );
    giProbeVB = device.CreateBuffer( ResourceType::VertexBuffer, HeapType::Default, false, vbSize, int( sizeof( GIProbeVertexFormat ) ), L"GIProbeVertices" );
    giProbeIB = device.CreateBuffer( ResourceType::IndexBuffer,  HeapType::Default, false, ibSize, int( sizeof( uint16_t ) ), L"GIProbeIndices" );

    auto ubVB = RenderManager::GetInstance().GetUploadConstantBufferForResource( *giProbeVB );
    auto ubIB = RenderManager::GetInstance().GetUploadConstantBufferForResource( *giProbeIB );
    commandList.UploadBufferResource( std::move( ubVB ), *giProbeVB, probeVertices.data(), vbSize );
    commandList.UploadBufferResource( std::move( ubIB ), *giProbeIB,  probeIndices.data(), ibSize );

    giProbeIndexCount = int( probeIndices.size() );
  }

  commandList.EndEvent();
}

void Scene::UpdateRaytracing( CommandList& commandList )
{
  if ( rtState == RTState::Ready )
    return;

  auto& device = RenderManager::GetInstance().GetDevice();

  std::vector< RTInstance > rtInstances;
  sceneAABB = BoundingBox();

  rtInstances.reserve( modelEntries.size() );
  for ( auto& entry : modelEntries )
  {
    BoundingBox::CreateMerged( sceneAABB, sceneAABB, entry.second.CalcBoundingBox() );

    for ( auto subset : entry.second.subsets )
    {
      auto meshAccel   = entry.second.mesh->GetRTAcceleratorForSubset( subset );
      auto entityAccel = entry.second.rtAccelerators[ subset ];
      auto usedAccel   = entityAccel ? entityAccel : meshAccel.accel;
      if ( usedAccel )
      {
        rtInstances.emplace_back();
        rtInstances.back().accel      = usedAccel;
        rtInstances.back().transform  = &entry.second.nodeTransform;
        rtInstances.back().rtIBSlot   = meshAccel.rtIBSlot;
        rtInstances.back().rtVBSlot   = meshAccel.rtVBSlot;
      }
    }
  }

  if ( rtState == RTState::TrianglesModified && rtDescriptor )
    rtDescriptor->Update( device, commandList, std::move( rtInstances ) );
  else
    rtDescriptor = device.CreateRTTopLevelAccelerator( commandList, std::move( rtInstances ) );

  rtState = RTState::Ready;
}

void Scene::RebuildLightCB( CommandList& commandList, const std::vector< LightCB >& lights )
{
  LightingEnvironmentParamsCB lightingEnvironmentParams;
  lightingEnvironmentParams.lightCount       = 1;
  lightingEnvironmentParams.skyMaterial      = skyMaterial;
  lightingEnvironmentParams.sceneLights[ 0 ] = directionalLight;

  for ( auto light : lights )
  {
    lightingEnvironmentParams.sceneLights[ lightingEnvironmentParams.lightCount ] = light;
    lightingEnvironmentParams.lightCount++;
  }

  auto ub = RenderManager::GetInstance().GetUploadConstantBufferForResource( *lightingConstantBuffer );
  commandList.UploadBufferResource( std::move( ub ), *lightingConstantBuffer, &lightingEnvironmentParams, sizeof( LightingEnvironmentParamsCB ) );
}