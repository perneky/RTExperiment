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
#include "MeasureCPUTime.h"
#include "Platform/Window.h"
#include "Common/Color.h"
#include "Common/Files.h"
#include "Icosahedron.h"
#include "Gizmo.h"
#include "Halton.h"

static constexpr int BlurParamsFloatCount = ( 17 + 17 + 1 ) * 4;

static constexpr float initialMinLog = -12.0f;
static constexpr float initialMaxLog = 4.0f;

static void InitializeManualExposure( CommandList& commandList, Resource& expBuffer, Resource& expOnlyBuffer, float exposure )
{
  ExposureBufferCB params;
  params.exposure        = exposure;
  params.invExposure     = 1.0f / exposure;
  params.targetExposure  = exposure;
  params.weightedHistAvg = 0;
  params.minLog          = initialMinLog;
  params.maxLog          = initialMaxLog;
  params.logRange        = initialMaxLog - initialMinLog;
  params.invLogRange     = 1.0f / params.logRange;

  auto uploadExposure = RenderManager::GetInstance().GetUploadConstantBufferForResource( expBuffer );
  commandList.UploadBufferResource( std::move( uploadExposure ), expBuffer, &params, sizeof( params ) );

  auto uploadExposureOnly = RenderManager::GetInstance().GetUploadConstantBufferForResource( expOnlyBuffer );
  commandList.UploadBufferResource( std::move( uploadExposureOnly ), expOnlyBuffer, &exposure, sizeof( exposure ) );
}

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
  ZeroMemory( &giArea, sizeof( giArea ) );
}

Scene::~Scene()
{
}

void Scene::TearDown( CommandList& commandList )
{
  if ( upscaling )
  {
    upscaling->TearDown( commandList );
    upscaling.reset();
  }
}

void Scene::SetUp( CommandList& commandList, Window& window )
{
  auto& renderManager  = RenderManager::GetInstance();
  auto& device         = renderManager.GetDevice();

  auto toneMappingFile       = ReadFileToMemory( L"Content/Shaders/ToneMapping.cso" );
  auto combineLightingFile   = ReadFileToMemory( L"Content/Shaders/CombineLighting.cso" );
  auto downsampleFile        = ReadFileToMemory( L"Content/Shaders/Downsample.cso" );
  auto downsample4File       = ReadFileToMemory( L"Content/Shaders/Downsample4.cso" );
  auto downsample4WLumaFile  = ReadFileToMemory( L"Content/Shaders/Downsample4WithLuminanceFilter.cso" );
  auto blurFile              = ReadFileToMemory( L"Content/Shaders/Blur.cso" );
  auto traceGIProbeFile      = ReadFileToMemory( L"Content/Shaders/TraceGIProbe.cso" );
  auto adaptExposureFile     = ReadFileToMemory( L"Content/Shaders/AdaptExposure.cso" );
  auto specBRDFLUTFile       = ReadFileToMemory( L"Content/Shaders/SpecBRDFLUT.cso" );
  auto processReflectionFile = ReadFileToMemory( L"Content/Shaders/ProcessReflection.cso" );
  auto extractBloomFile      = ReadFileToMemory( L"Content/Shaders/ExtractBloom.cso" );
  auto blurBloomFile         = ReadFileToMemory( L"Content/Shaders/BlurBloom.cso" );
  auto extractLumaFile       = ReadFileToMemory( L"Content/Shaders/ExtractLuma.cso" );
  auto generateHistogramFile = ReadFileToMemory( L"Content/Shaders/GenerateHistogram.cso" );
  auto debugHistogramFile    = ReadFileToMemory( L"Content/Shaders/DebugDrawHistogram.cso" );
  auto downsampleBloomFile   = ReadFileToMemory( L"Content/Shaders/DownsampleBloom.cso" );
  auto upsampleBlurBloomFile = ReadFileToMemory( L"Content/Shaders/UpsampleAndBlurBloom.cso" );

  float zero = 0;

  histogramBuffer         = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, true,  256 * sizeof( uint32_t ), sizeof( uint32_t ), L"Histogram" );
  exposureBuffer          = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, true,  sizeof( ExposureBufferCB ), sizeof( ExposureBufferCB ), L"Exposure" );
  frameConstantBuffer     = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( FrameParamsCB ), sizeof( FrameParamsCB ), L"FrameParams" );
  prevFrameConstantBuffer = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( FrameParamsCB ), sizeof( FrameParamsCB ), L"PrevFrameParams" );
  lightingConstantBuffer  = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( LightingEnvironmentParamsCB ), sizeof( LightingEnvironmentParamsCB ), L"LightingParams" );
  allMeshParamsBuffer     = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( MeshParamsCB ) * MaxInstanceCount, sizeof( MeshParamsCB ), L"AllMeshParams" );
  haltonSequenceBuffer    = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( HaltonSequenceCB ), sizeof( HaltonSequenceCB ), L"HaltonSequence" );
  toneMappingShader       = device.CreateComputeShader( toneMappingFile.data(), int( toneMappingFile.size() ), L"ToneMapping" );
  combineLightingShader   = device.CreateComputeShader( combineLightingFile.data(), int( combineLightingFile.size() ), L"CombineLighting" );
  downsampleShader        = device.CreateComputeShader( downsampleFile.data(), int( downsampleFile.size() ), L"Downsample" );
  downsample4Shader       = device.CreateComputeShader( downsample4File.data(), int( downsample4File.size() ), L"Downsample4" );
  downsample4WLumaShader  = device.CreateComputeShader( downsample4WLumaFile.data(), int( downsample4WLumaFile.size() ), L"Downsample4WLuma" );
  blurShader              = device.CreateComputeShader( blurFile.data(), int( blurFile.size() ), L"Blur" );
  traceGIProbeShader      = device.CreateComputeShader( traceGIProbeFile.data(), int( traceGIProbeFile.size() ), L"TraceGIProbe" );
  adaptExposureShader     = device.CreateComputeShader( adaptExposureFile.data(), int( adaptExposureFile.size() ), L"AdaptExposure" );
  specBRDFLUTShader       = device.CreateComputeShader( specBRDFLUTFile.data(), int( specBRDFLUTFile.size() ), L"SpecBRDFLUT" );
  processReflectionShader = device.CreateComputeShader( processReflectionFile.data(), int( processReflectionFile.size() ), L"ProcessReflection" );
  extractBloomShader      = device.CreateComputeShader( extractBloomFile.data(), int( extractBloomFile.size() ), L"ExtractBloom" );
  blurBloomShader         = device.CreateComputeShader( blurBloomFile.data(), int( blurBloomFile.size() ), L"BlurBloom" );
  extractLumaShader       = device.CreateComputeShader( extractLumaFile.data(), int( extractLumaFile.size() ), L"ExtractLuma" );
  generateHistogramShader = device.CreateComputeShader( generateHistogramFile.data(), int( generateHistogramFile.size() ), L"GenerateHistogram" );
  debugHistogramShader    = device.CreateComputeShader( debugHistogramFile.data(), int( debugHistogramFile.size() ), L"DebugHistogram" );
  downsampleBloomShader   = device.CreateComputeShader( downsampleBloomFile.data(), int( downsampleBloomFile.size() ), L"DownsampleBloom" );
  upsampleBlurBloomShader = device.CreateComputeShader( upsampleBlurBloomFile.data(), int( upsampleBlurBloomFile.size() ), L"UpsampleBlurBloom" );
  exposureOnlyBuffer      = device.Create2DTexture( commandList, 1, 1, nullptr, 0, PixelFormat::R32F, true, -1, -1, false, L"ExposureOnly" );

  auto allMeshParamsDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, AllMeshParamsSlot, *allMeshParamsBuffer, sizeof( MeshParamsCB ) );
  allMeshParamsBuffer->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( allMeshParamsDesc ) );

  auto exposureBufferDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, -1, *exposureBuffer, sizeof( ExposureBufferCB ) );
  exposureBuffer->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( exposureBufferDesc ) );

  auto exposureBufferUAVDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::UnorderedAccessView, -1, *exposureBuffer, sizeof( ExposureBufferCB ) );
  exposureBuffer->AttachResourceDescriptor( ResourceDescriptorType::UnorderedAccessView, std::move( exposureBufferUAVDesc ) );

  auto histogramBufferDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, -1, *histogramBuffer, sizeof( uint32_t ) );
  histogramBuffer->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( histogramBufferDesc ) );

  auto histogramBufferUAVDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::UnorderedAccessView, -1, *histogramBuffer, sizeof( uint32_t ) );
  histogramBuffer->AttachResourceDescriptor( ResourceDescriptorType::UnorderedAccessView, std::move( histogramBufferUAVDesc ) );

  InitializeManualExposure( commandList, *exposureBuffer, *exposureOnlyBuffer, exposure );

  RecreateWindowSizeDependantResources( commandList, window );

  giTimer = std::make_unique< MeasureCPUTime >( device );

  HaltonSequenceCB hsb;
  int haltonBase[] = { 2, 3 };
  for ( int hix = 0; hix < HaltonSequenceLength; ++hix )
  {
    auto vals = halton_base( hix + 1, 2, haltonBase );

    hsb.values[ hix ].x = vals[ 0 ];
    hsb.values[ hix ].y = vals[ 1 ];
    hsb.values[ hix ].z = vals[ 0 ] * 2 - 1;
    hsb.values[ hix ].w = vals[ 1 ] * 2 - 1;

#ifdef _DEBUG
    for ( int cix = 0; cix < hix; ++cix )
    {
      float dx = abs( hsb.values[ hix ].x - hsb.values[ cix ].x );
      float dy = abs( hsb.values[ hix ].y - hsb.values[ cix ].y );
      assert( !( dx < 0.001f && dy < 0x001f ) );
    }
#endif // _DEBUG
  }

  auto hub = RenderManager::GetInstance().GetUploadConstantBufferForResource( *haltonSequenceBuffer );
  commandList.UploadBufferResource( std::move( hub ), *haltonSequenceBuffer, &hsb, sizeof( HaltonSequenceCB ) );
}

void Scene::CreateBRDFLUTTexture( CommandList& commandList )
{
  if ( specBRDFLUTTexture )
    return;

  specBRDFLUTTexture = RenderManager::GetInstance().GetDevice().Create2DTexture( commandList, SpecBRDFLUTSize, SpecBRDFLUTSize, nullptr, 0, PixelFormat::RG1616F, false, SpecBRDFLUTSlot, SpecBRDFLUTUAVSlot, false, L"SpecBRDFLUT" );

  commandList.ChangeResourceState( *specBRDFLUTTexture, ResourceStateBits::UnorderedAccess );
  commandList.SetComputeShader( *specBRDFLUTShader );
  commandList.SetComputeResource( 0, *specBRDFLUTTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
  commandList.Dispatch( SpecBRDFLUTSize / 32, SpecBRDFLUTSize / 32, 1 );
  commandList.AddUAVBarrier( *specBRDFLUTTexture );
  commandList.ChangeResourceState( *specBRDFLUTTexture, ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );
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
    SetDirectionalLight( XMVector3Normalize( XMLoadFloat3( &sky.lightDirection ) ), 1000, 10, XMLoadFloat3( &sky.lightColor ), sky.lightIntensity, true );
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
  directionalLight.scatterShadow    = true;
  XMStoreFloat4( &directionalLight.direction, direction );
  XMStoreFloat4( &directionalLight.color, color * intensity );
}

void Scene::SetCamera( Camera& camera )
{
  // TODO: some preceise clip plane calculation
  float nearZ = 0.1f;
  float farZ  = 200.0f;

  XMFLOAT2 jitter = XMFLOAT2( 0, 0 );
  if ( upscaling )
  {
    jitter    = upscaling->GetJitter();
    jitter.x *=  2.0f / lowResHDRTexture->GetTextureWidth ();
    jitter.y *= -2.0f / lowResHDRTexture->GetTextureHeight();
  }
  camera.SetJitter( jitter );
  camera.SetDepthRange( nearZ, farZ );

  XMStoreFloat4x4( &frameParams.vpTransform,              camera.GetViewProjectionTransform( true ) );
  XMStoreFloat4x4( &frameParams.vpTransformNoJitter,      camera.GetViewProjectionTransform( false ) );
  XMStoreFloat4x4( &frameParams.viewTransform,            camera.GetViewTransform() );
  XMStoreFloat4x4( &frameParams.projTransform,            camera.GetProjectionTransform( true ) );
  XMStoreFloat4x4( &frameParams.invProjTransform,         XMMatrixInverse( nullptr, camera.GetProjectionTransform( true ) ) );
  XMStoreFloat4x4( &frameParams.projTransformNoJitter,    camera.GetProjectionTransform( false ) );
  XMStoreFloat4x4( &frameParams.invProjTransformNoJitter, XMMatrixInverse( nullptr, camera.GetProjectionTransform( false ) ) );
  XMStoreFloat4  ( &frameParams.cameraPosition,           camera.GetPosition() );
  XMStoreFloat4  ( &frameParams.cameraDirection,          camera.GetDirection() );
  frameParams.screenSizeLQ.x = float( lowResHDRTexture->GetTextureWidth() );
  frameParams.screenSizeLQ.y = float( lowResHDRTexture->GetTextureHeight() );
  frameParams.screenSizeLQ.z = 1.0f / frameParams.screenSizeLQ.x;
  frameParams.screenSizeLQ.w = 1.0f / frameParams.screenSizeLQ.y;
  frameParams.screenSizeHQ.x = float( GetTargetWidth () );
  frameParams.screenSizeHQ.y = float( GetTargetHeight() );
  frameParams.screenSizeHQ.z = 1.0f / frameParams.screenSizeHQ.x;
  frameParams.screenSizeHQ.w = 1.0f / frameParams.screenSizeHQ.y;
  frameParams.jitter         = upscaling ? upscaling->GetJitter() : XMFLOAT2( 0, 0 );
  frameParams.nearFarPlane.x = camera.GetNearDepth();
  frameParams.nearFarPlane.y = camera.GetFarDepth();
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

  auto& renderManager           = RenderManager::GetInstance();
  auto  vTransform              = XMLoadFloat4x4( &frameParams.viewTransform );
  auto  pTransform              = XMLoadFloat4x4( &frameParams.projTransform );
  auto  vpTransform             = XMLoadFloat4x4( &frameParams.vpTransform );
  auto  vpTransformNoJitter     = XMLoadFloat4x4( &frameParams.vpTransformNoJitter );
  auto  prevVTransform          = XMLoadFloat4x4( &prevFrameParams.viewTransform );
  auto  prevVPTransform         = XMLoadFloat4x4( &prevFrameParams.vpTransform );
  auto  prevVPTransformNoJitter = XMLoadFloat4x4( &prevFrameParams.vpTransformNoJitter );
  auto  prevTargetIndex         = currentTargetIndex;

  currentTargetIndex = 1 - currentTargetIndex;
  currentGISource    = 1 - currentGISource;

  frameParams.frameIndex  = ( frameParams.frameIndex + 1 ) % 10000;
  frameParams.depthIndex  = ( currentTargetIndex == 0 ? DepthSlot0          : DepthSlot1          ) - BaseSlot;
  frameParams.aoIndex     = ( currentTargetIndex == 0 ? DirectLighting1Slot : DirectLighting2Slot ) - BaseSlot;
  frameParams.hqsIndex    = ( currentTargetIndex == 0 ? HQS1Slot            : HQS2Slot            ) - BaseSlot;

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

  auto& hqsTexture     = hqsTextures[ currentTargetIndex ];
  auto& prevHQSTexture = hqsTextures[ prevTargetIndex ];

  auto& finalHDRTexture = highResHDRTexture ? highResHDRTexture : lowResHDRTexture;

  auto renderWidthLQ  = lowResHDRTexture->GetTextureWidth();
  auto renderHeightLQ = lowResHDRTexture->GetTextureHeight();
  auto renderWidthHQ  = GetTargetWidth ();
  auto renderHeightHQ = GetTargetHeight();

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
                                          , vpTransformNoJitter
                                          , prevModelTransform
                                          , prevVPTransform
                                          , prevVPTransformNoJitter
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

  CreateBRDFLUTTexture( commandList );

  commandList.BeginEvent( 0, L"Scene::Render(All mesh params)" );

  auto allMeshParamsSize = int( sizeof( MeshParamsCB ) * allMeshParams.size() );
  auto uploadAllMeshParamsBuffer = RenderManager::GetInstance().GetUploadConstantBufferForResource( *allMeshParamsBuffer );
  commandList.UploadBufferResource( std::move( uploadAllMeshParamsBuffer ), *allMeshParamsBuffer, allMeshParams.data(), allMeshParamsSize );
  commandList.ChangeResourceState( *allMeshParamsBuffer, ResourceStateBits::NonPixelShaderInput | ResourceStateBits::PixelShaderInput );

  renderManager.GetBLASGPUInfoResource( commandList );

  commandList.EndEvent();

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
    commandList.SetConstantBuffer( 3, *prevFrameConstantBuffer );
    commandList.SetDescriptorHeap( 4, renderManager.GetShaderResourceHeap(), 0 );
    commandList.SetDescriptorHeap( 5, renderManager.GetSamplerHeap(), 0 );

    renderManager.BindAllMaterials( commandList, 2 );
  };

  auto setUpOpaquePass = [&]( PipelinePresets preset )
  {
    commandList.SetPipelineState( renderManager.GetPipelinePreset( preset ) );
    commandList.SetConstantBuffer( 1, *frameConstantBuffer );
    commandList.SetConstantBuffer( 2, *prevFrameConstantBuffer );
    commandList.SetConstantBuffer( 3, *lightingConstantBuffer );
    commandList.SetConstantBuffer( 5, *haltonSequenceBuffer );
    commandList.SetDescriptorHeap( 6, renderManager.GetShaderResourceHeap(), 0 );
    commandList.SetRayTracingScene( 7, *rtScene );
    commandList.SetDescriptorHeap( 8, renderManager.GetSamplerHeap(), 0 );

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
    commandList.SetComputeConstantBuffer( 1, *lightingConstantBuffer );
    commandList.SetComputeConstantBuffer( 2, *frameConstantBuffer );
    commandList.SetComputeConstantBuffer( 3, *prevFrameConstantBuffer );
    renderManager.BindAllMaterialsToCompute( commandList, 4 );
    commandList.SetComputeConstantBuffer( 5, *haltonSequenceBuffer );
    commandList.SetComputeResource( 6, *giProbeTextures[ currentGISource ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.SetComputeTextureHeap( 7, renderManager.GetShaderResourceHeap(), 0 );
    commandList.SetComputeRayTracingScene( 8, *rtScene );

    if ( giTimer->GetStatus() == MeasureCPUTime::Status::Stopped )
    {
      auto giTime = giTimer->GetResult( commandList );
      if ( giTime > 0 )
      {
        giProbeUpdatePerFrame += giTime > giProbeUpdatePerFrameTimeBudget  ? -10 : 10;

        // static char giTimeMessage[ 1024 ] = {};
        // sprintf_s( giTimeMessage, "giProbeUpdatePerFrame: %d, time: %f\n", giProbeUpdatePerFrame, giTime );
        // OutputDebugStringA( giTimeMessage );
      }

      if ( giTime >= 0 )
        giTimer->Reset();
    }

    if ( giTimer->GetStatus() == MeasureCPUTime::Status::Idle )
      giTimer->Start( commandList );

    commandList.Dispatch( giProbeUpdatePerFrame, 1, 1 );

    if ( giTimer->GetStatus() == MeasureCPUTime::Status::Started )
      giTimer->Stop( commandList );

    commandList.AddUAVBarrier( *giProbeTextures[ currentGISource ] );
    commandList.ChangeResourceState( *giProbeTextures[ currentGISource ], ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );

    giProbeUpdateNext = ( giProbeUpdateNext + giProbeUpdatePerFrame ) % ( frameParams.giProbeCount.x * frameParams.giProbeCount.y * frameParams.giProbeCount.z );

    commandList.EndEvent();
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(Back buffer setup)" );

    commandList.ChangeResourceState( *sdrTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *hqsTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *directLightingTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *indirectLightingTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *reflectionTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *depthTexture, ResourceStateBits::DepthWrite );
    commandList.ChangeResourceState( *prevDepthTexture, ResourceStateBits::DepthRead | ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );

    commandList.SetViewport( 0, 0, renderWidthLQ, renderHeightLQ );
    commandList.SetScissor ( 0, 0, renderWidthLQ, renderHeightLQ );

    commandList.Clear( *depthTexture, 1 );

    commandList.EndEvent();
  }

  if ( upscaling )
  {
    commandList.ChangeResourceState( *motionVectorTexture, ResourceStateBits::RenderTarget );
    commandList.Clear( *motionVectorTexture, Color() );
    commandList.SetRenderTarget( *motionVectorTexture, depthTexture.get() );
  }
  else
    commandList.SetRenderTarget( {}, depthTexture.get() );

  if ( !visibleEntries.empty() )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Depth pass opaque)" );

    setUpDepthPass( upscaling ? PipelinePresets::DepthWithMotionVectors : PipelinePresets::Depth );

    for ( auto& entry : visibleEntries )
      if ( !entry.second.entry.mesh->IsSkin() )
        renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::Opaque );

    if ( hasSkinned )
    {
      setUpDepthPass( upscaling ? PipelinePresets::DepthWithHistoryWithMotionVectors : PipelinePresets::DepthWithHistory );

      for ( auto& entry : visibleEntries )
        if ( entry.second.entry.mesh->IsSkin() )
          renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::Opaque );

    }

    commandList.EndEvent();
  }

  if ( !visibleEntries.empty() )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Depth pass one bit alpha)" );

    setUpDepthPass( upscaling ? PipelinePresets::DepthOneBitAlphaWithMotionVectors : PipelinePresets::DepthOneBitAlpha );

    for ( auto& entry : visibleEntries )
      if ( !entry.second.entry.mesh->IsSkin() )
        renderMesh( entry.first, entry.second.entry, entry.second.index, AlphaModeCB::OneBitAlpha );

    if ( hasSkinned )
    {
      setUpDepthPass( upscaling ? PipelinePresets::DepthOneBitAlphaWithHistoryWithMotionVectors : PipelinePresets::DepthOneBitAlphaWithHistory );

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

    commandList.ChangeResourceState( *hqsTexture, ResourceStateBits::RenderTarget );
    commandList.ChangeResourceState( *prevHQSTexture, ResourceStateBits::PixelShaderInput | ResourceStateBits::NonPixelShaderInput );

    commandList.Clear( *directLightingTexture, Color( 0, 0, 0, 0 ) );
    commandList.Clear( *indirectLightingTexture, Color( 0, 0, 0, 0 ) );
    commandList.Clear( *hqsTexture, Color( 0, 0, 0, 0 ) );
    commandList.Clear( *reflectionTexture, Color( 0, 0, 0, 0 ) );

    commandList.SetRenderTarget( { directLightingTexture.get()
                                 , indirectLightingTexture.get()
                                 , reflectionTexture.get() 
                                 , hqsTexture.get() }
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

    commandList.SetComputeShader( *downsample4WLumaShader );
    commandList.SetComputeResource( 0, *reflectionTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 1, *reflectionProcTextures[ 0 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( reflectionProcTextures[ 0 ]->GetTextureWidth () + DownsamplingKernelWidth  - 1 ) / DownsamplingKernelWidth
                        , ( reflectionProcTextures[ 0 ]->GetTextureHeight() + DownsamplingKernelHeight - 1 ) / DownsamplingKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *reflectionProcTextures[ 0 ] );

    commandList.ChangeResourceState( *depthTexture, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *reflectionProcDepth, ResourceStateBits::UnorderedAccess );

    commandList.SetComputeShader( *downsample4Shader );
    commandList.SetComputeResource( 0, *depthTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 1, *reflectionProcDepth->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( reflectionProcDepth->GetTextureWidth () + DownsamplingKernelWidth  - 1 ) / DownsamplingKernelWidth
                        , ( reflectionProcDepth->GetTextureHeight() + DownsamplingKernelHeight - 1 ) / DownsamplingKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *reflectionProcDepth );

    //=================================================================================================

    commandList.ChangeResourceState( *reflectionProcDepth, ResourceStateBits::NonPixelShaderInput );

    commandList.SetComputeShader( *processReflectionShader );
    commandList.SetComputeConstantBuffer( 1, *haltonSequenceBuffer );
    commandList.SetComputeResource( 2, *reflectionProcDepth->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );

    int sourceIx = 0;

    struct
    {
      XMFLOAT4X4 invProj;
      XMFLOAT4   cornerWorldDirs[ 4 ];
      XMFLOAT4   cameraPosition;
      int        texWidth;
      int        texHeight;
      float      invTexWidth;
      float      invTexHeight;
      int        dir;
    } params;

    float sw = float( depthTexture->GetTextureWidth () );
    float sh = float( depthTexture->GetTextureHeight() );

    auto cameraPosition = XMLoadFloat4( &frameParams.cameraPosition );
    XMStoreFloat4( &params.cornerWorldDirs[ 0 ], XMVector3Unproject( XMVectorSet(  0,  0, 1, 1 ), 0, 0, sw, sh, 0, 1, pTransform, vTransform, XMMatrixIdentity() ) - cameraPosition );
    XMStoreFloat4( &params.cornerWorldDirs[ 1 ], XMVector3Unproject( XMVectorSet( sw,  0, 1, 1 ), 0, 0, sw, sh, 0, 1, pTransform, vTransform, XMMatrixIdentity() ) - cameraPosition );
    XMStoreFloat4( &params.cornerWorldDirs[ 2 ], XMVector3Unproject( XMVectorSet(  0, sh, 1, 1 ), 0, 0, sw, sh, 0, 1, pTransform, vTransform, XMMatrixIdentity() ) - cameraPosition );
    XMStoreFloat4( &params.cornerWorldDirs[ 3 ], XMVector3Unproject( XMVectorSet( sw, sh, 1, 1 ), 0, 0, sw, sh, 0, 1, pTransform, vTransform, XMMatrixIdentity() ) - cameraPosition );

    params.cameraPosition = frameParams.cameraPosition;

    params.texWidth     = reflectionProcTextures[ 0 ]->GetTextureWidth();
    params.texHeight    = reflectionProcTextures[ 0 ]->GetTextureHeight();
    params.invTexWidth  = 1.0f / reflectionProcTextures[ 0 ]->GetTextureWidth();
    params.invTexHeight = 1.0f / reflectionProcTextures[ 0 ]->GetTextureHeight();
    params.invProj      = frameParams.invProjTransform;

    for ( int iter = 0; iter < 2; ++iter )
    {
      params.dir = iter;

      commandList.ChangeResourceState( *reflectionProcTextures[ sourceIx ], ResourceStateBits::NonPixelShaderInput );
      commandList.ChangeResourceState( *reflectionProcTextures[ 1 - sourceIx ], ResourceStateBits::UnorderedAccess );

      commandList.SetComputeResource( 3, *reflectionProcTextures[ sourceIx ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
      commandList.SetComputeResource( 4, *reflectionProcTextures[ 1 - sourceIx ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );

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
    commandList.ChangeResourceState( *lowResHDRTexture, ResourceStateBits::UnorderedAccess );

    struct
    {
      float            invTexWidth;
      float            invTexHeight;
      FrameDebugModeCB debugMode;
    } params;

    params.invTexWidth  = 1.0f / lowResHDRTexture->GetTextureWidth();
    params.invTexHeight = 1.0f / lowResHDRTexture->GetTextureHeight();
    params.debugMode    = frameParams.frameDebugMode;

    commandList.SetComputeShader( *combineLightingShader );
    commandList.SetComputeConstantValues( 0, params );
    commandList.SetComputeResource( 1, *directLightingTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 2, *indirectLightingTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 3, *reflectionTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 4, *reflectionProcTextures[ finalReflectionProcIndex ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 5, *lowResHDRTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( lowResHDRTexture->GetTextureWidth () + LightCombinerKernelWidth - 1  ) / LightCombinerKernelWidth
                        , ( lowResHDRTexture->GetTextureHeight() + LightCombinerKernelHeight - 1 ) / LightCombinerKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *lowResHDRTexture );

    commandList.EndEvent();
  }

  if ( skybox )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Skybox)" );

    commandList.ChangeResourceState( *lowResHDRTexture, ResourceStateBits::RenderTarget );
    commandList.SetRenderTarget( *lowResHDRTexture, depthTexture.get() );

    commandList.SetPipelineState( renderManager.GetPipelinePreset( PipelinePresets::Skybox ) );
    commandList.SetConstantBuffer( 1, *frameConstantBuffer );
    commandList.SetDescriptorHeap( 3, renderManager.GetShaderResourceHeap(), 0 );

    renderManager.BindAllMaterials( commandList, 2 );

    skybox->Render( renderManager.GetDevice(), commandList, 0, { 0 }, AlphaModeCB::Opaque, nullptr, PrimitiveType::TriangleStrip );

    commandList.EndEvent();
  }

  if ( editorInfo.showGIProbes && giProbeTextures[ currentGISource ] )
  {
    commandList.BeginEvent( 0, L"Scene::Render(GI probes)" );

    commandList.SetPipelineState( renderManager.GetPipelinePreset( PipelinePresets::GIProbes ) );
    commandList.SetConstantValues( 0, frameParams.vpTransform );
    commandList.SetConstantBuffer( 1, *frameConstantBuffer );
    commandList.SetDescriptorHeap( 2, renderManager.GetShaderResourceHeap(), 0 );

    commandList.SetVertexBuffer( *giProbeVB );
    commandList.SetIndexBuffer( *giProbeIB );

    commandList.SetPrimitiveType( PrimitiveType::TriangleList );
    commandList.DrawIndexed( giProbeIndexCount, frameParams.giProbeCount.x * frameParams.giProbeCount.y * frameParams.giProbeCount.z );

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

  if ( editorInfo.renderLightMarkers )
  {
    commandList.BeginEvent( 0, L"Scene::Render(LightMarkers)" );

    renderManager.BeginSpriteRendering( commandList );

    for ( int lightIx = 0; lightIx < lightingEnvironmentParams.lightCount; ++lightIx )
    {
      auto& light = lightingEnvironmentParams.sceneLights[ lightIx ];
      switch ( light.type )
      {
      case LightTypeCB::Point:
        renderManager.AddSprite( commandList, XMLoadFloat4( &light.origin ), RenderManager::Sprites::Light );
        break;
      case LightTypeCB::Spot:
        renderManager.AddSprite( commandList, XMLoadFloat4( &light.origin ), RenderManager::Sprites::Light );
        break;
      }
    }

    renderManager.FinishSpriteRendering( commandList, vTransform, pTransform );

    commandList.EndEvent();
  }

  if ( upscaling )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Upscaling)" );

    upscaling->Upscale( commandList
                      , *lowResHDRTexture
                      , *depthTexture
                      , *motionVectorTexture
                      , *highResHDRTexture
                      , *exposureOnlyBuffer
                      , upscaling->GetJitter() );

    commandList.SetViewport( 0, 0, renderWidthHQ, renderHeightHQ );
    commandList.SetScissor ( 0, 0, renderWidthHQ, renderHeightHQ );

    commandList.ChangeResourceState( *highResHDRTexture, ResourceStateBits::RenderTarget );
    commandList.SetRenderTarget( *highResHDRTexture, nullptr );

    commandList.EndEvent();
  }

  {
    {
      commandList.BeginEvent( 0, L"Scene::Render(Extract bloom)" );

      commandList.ChangeResourceState( *finalHDRTexture, ResourceStateBits::NonPixelShaderInput );
      commandList.ChangeResourceState( *exposureBuffer, ResourceStateBits::VertexOrConstantBuffer );
      commandList.ChangeResourceState( *bloomTextures[ 0 ][ 0 ], ResourceStateBits::UnorderedAccess );
      commandList.ChangeResourceState( *lumaTexture, ResourceStateBits::UnorderedAccess );

      struct
      {
        float invOutputWidth;
        float invOutputHeight;
        float bloomThreshold;
      } params;

      params.invOutputWidth  = 1.0f / bloomTextures[ 0 ][ 0 ]->GetTextureWidth();
      params.invOutputHeight = 1.0f / bloomTextures[ 0 ][ 0 ]->GetTextureHeight();
      params.bloomThreshold  = bloomThreshold;

      commandList.SetComputeShader( *extractBloomShader );
      commandList.SetComputeConstantValues( 0, params );
      commandList.SetComputeConstantBuffer( 1, *exposureBuffer );
      commandList.SetComputeResource( 2, *finalHDRTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
      commandList.SetComputeResource( 3, *bloomTextures[ 0 ][ 0 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
      commandList.SetComputeResource( 4, *lumaTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
      commandList.Dispatch( ( bloomTextures[ 0 ][ 0 ]->GetTextureWidth () + ExtractBloomKernelWidth - 1  ) / ExtractBloomKernelWidth
                          , ( bloomTextures[ 0 ][ 0 ]->GetTextureHeight() + ExtractBloomKernelHeight - 1 ) / ExtractBloomKernelHeight
                          , 1 );

      commandList.AddUAVBarrier( *bloomTextures[ 0 ][ 0 ] );
      commandList.AddUAVBarrier( *lumaTexture );

      commandList.EndEvent();
    }

    {
      commandList.BeginEvent( 0, L"Scene::Render(Downsample bloom)" );

      commandList.ChangeResourceState( *bloomTextures[ 0 ][ 0 ], ResourceStateBits::NonPixelShaderInput );
      commandList.ChangeResourceState( *bloomTextures[ 1 ][ 0 ], ResourceStateBits::UnorderedAccess );
      commandList.ChangeResourceState( *bloomTextures[ 2 ][ 0 ], ResourceStateBits::UnorderedAccess );
      commandList.ChangeResourceState( *bloomTextures[ 3 ][ 0 ], ResourceStateBits::UnorderedAccess );
      commandList.ChangeResourceState( *bloomTextures[ 4 ][ 0 ], ResourceStateBits::UnorderedAccess );

      struct
      {
        float invOutputWidth;
        float invOutputHeight;
      } params;

      params.invOutputWidth  = 1.0f / bloomTextures[ 0 ][ 0 ]->GetTextureWidth();
      params.invOutputHeight = 1.0f / bloomTextures[ 0 ][ 0 ]->GetTextureHeight();

      commandList.SetComputeShader( *downsampleBloomShader );
      commandList.SetComputeConstantValues( 0, params );
      commandList.SetComputeResource( 1, *bloomTextures[ 0 ][ 0 ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
      commandList.SetComputeResource( 2, *bloomTextures[ 1 ][ 0 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
      commandList.SetComputeResource( 3, *bloomTextures[ 2 ][ 0 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
      commandList.SetComputeResource( 4, *bloomTextures[ 3 ][ 0 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
      commandList.SetComputeResource( 5, *bloomTextures[ 4 ][ 0 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
      commandList.Dispatch( ( bloomTextures[ 0 ][ 0 ]->GetTextureWidth () / 2 + ExtractBloomKernelWidth - 1  ) / ExtractBloomKernelWidth
                          , ( bloomTextures[ 0 ][ 0 ]->GetTextureHeight() / 2 + ExtractBloomKernelHeight - 1 ) / ExtractBloomKernelHeight
                          , 1 );

      commandList.AddUAVBarrier( *bloomTextures[ 1 ][ 0 ] );
      commandList.AddUAVBarrier( *bloomTextures[ 2 ][ 0 ] );
      commandList.AddUAVBarrier( *bloomTextures[ 3 ][ 0 ] );
      commandList.AddUAVBarrier( *bloomTextures[ 4 ][ 0 ] );

      commandList.EndEvent();
    }

    {
      commandList.BeginEvent( 0, L"Scene::Render(Blur bloom)" );

      commandList.ChangeResourceState( *bloomTextures[ 4 ][ 0 ], ResourceStateBits::NonPixelShaderInput );
      commandList.ChangeResourceState( *bloomTextures[ 4 ][ 1 ], ResourceStateBits::UnorderedAccess );

      struct
      {
        float invOutputWidth;
        float invOutputHeight;
      } params;

      params.invOutputWidth  = 1.0f / bloomTextures[ 4 ][ 0 ]->GetTextureWidth();
      params.invOutputHeight = 1.0f / bloomTextures[ 4 ][ 0 ]->GetTextureHeight();

      commandList.SetComputeShader( *blurBloomShader );
      commandList.SetComputeConstantValues( 0, params );
      commandList.SetComputeResource( 1, *bloomTextures[ 4 ][ 0 ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
      commandList.SetComputeResource( 2, *bloomTextures[ 4 ][ 1 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
      commandList.Dispatch( ( bloomTextures[ 4 ][ 0 ]->GetTextureWidth () + ExtractBloomKernelWidth - 1  ) / ExtractBloomKernelWidth
                          , ( bloomTextures[ 4 ][ 0 ]->GetTextureHeight() + ExtractBloomKernelHeight - 1 ) / ExtractBloomKernelHeight
                          , 1 );

      commandList.AddUAVBarrier( *bloomTextures[ 4 ][ 1 ] );

      commandList.EndEvent();
    }

    for ( int us = 0; us < 4; ++us )
    {
      commandList.BeginEvent( 0, L"Scene::Render(Upsample blur bloom)" );

      commandList.ChangeResourceState( *bloomTextures[ 3 - us ][ 0 ], ResourceStateBits::NonPixelShaderInput );
      commandList.ChangeResourceState( *bloomTextures[ 4 - us ][ 1 ], ResourceStateBits::NonPixelShaderInput );
      commandList.ChangeResourceState( *bloomTextures[ 3 - us ][ 1 ], ResourceStateBits::UnorderedAccess );

      struct
      {
        float invOutputWidth;
        float invOutputHeight;
        float upsampleBlendFactor;
      } params;

      params.invOutputWidth      = 1.0f / bloomTextures[ 3 - us ][ 0 ]->GetTextureWidth();
      params.invOutputHeight     = 1.0f / bloomTextures[ 3 - us ][ 0 ]->GetTextureHeight();
      params.upsampleBlendFactor = 0.65f;

      commandList.SetComputeShader( *upsampleBlurBloomShader );
      commandList.SetComputeConstantValues( 0, params );
      commandList.SetComputeResource( 1, *bloomTextures[ 3 - us ][ 0 ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
      commandList.SetComputeResource( 2, *bloomTextures[ 4 - us ][ 1 ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
      commandList.SetComputeResource( 3, *bloomTextures[ 3 - us ][ 1 ]->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
      commandList.Dispatch( ( bloomTextures[ 3 - us ][ 0 ]->GetTextureWidth () + ExtractBloomKernelWidth - 1  ) / ExtractBloomKernelWidth
                          , ( bloomTextures[ 3 - us ][ 0 ]->GetTextureHeight() + ExtractBloomKernelHeight - 1 ) / ExtractBloomKernelHeight
                          , 1 );

      commandList.AddUAVBarrier( *bloomTextures[ 3 - us ][ 1 ] );

      commandList.EndEvent();
    }
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(ToneMapping)" );

    commandList.ChangeResourceState( *exposureBuffer, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *finalHDRTexture, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *bloomTextures[ 0 ][ 1 ], ResourceStateBits::NonPixelShaderInput );
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
    commandList.SetComputeConstantValues( 0, params );
    commandList.SetComputeConstantBuffer( 1, *exposureBuffer );
    commandList.SetComputeResource( 2, *finalHDRTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 3, *bloomTextures[ 0 ][ 1 ]->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 4, *sdrTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( ( GetTargetWidth () + ToneMappingKernelWidth  - 1 ) / ToneMappingKernelWidth
                        , ( GetTargetHeight() + ToneMappingKernelHeight - 1 ) / ToneMappingKernelHeight
                        , 1 );

    commandList.AddUAVBarrier( *sdrTexture );

    commandList.EndEvent();
  }

  {
    commandList.BeginEvent( 0, L"Scene::Render(Adapt exposure)" );
    
    if ( !enableAdaptation || editorInfo.frameDebugMode != FrameDebugModeCB::None )
    {
      commandList.BeginEvent( 0, L"Scene::Render(Manual exposure)" );
      InitializeManualExposure( commandList, *exposureBuffer, *exposureOnlyBuffer, exposure );
      commandList.EndEvent();
    }
    else
    {
      {
        commandList.BeginEvent( 0, L"Scene::Render(Clear histogram)" );

        uint32_t zeroes[ 256 ] = { 0 };
        auto clearHistoUpload = RenderManager::GetInstance().GetUploadConstantBufferForResource( *histogramBuffer );
        commandList.UploadBufferResource( std::move( clearHistoUpload ), *histogramBuffer, zeroes, sizeof( zeroes ) );

        commandList.EndEvent();

        commandList.BeginEvent( 0, L"Scene::Render(Build histogram)" );

        commandList.ChangeResourceState( *lumaTexture, ResourceStateBits::NonPixelShaderInput );
        commandList.ChangeResourceState( *histogramBuffer, ResourceStateBits::UnorderedAccess );

        commandList.SetComputeShader( *generateHistogramShader );
        commandList.SetComputeResource( 0, *lumaTexture->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
        commandList.SetComputeResource( 1, *histogramBuffer->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
        commandList.Dispatch( ( lumaTexture->GetTextureWidth()  +  16 - 1 ) /  16
                            , ( lumaTexture->GetTextureHeight() + 384 - 1 ) / 384
                            , 1 );

        commandList.AddUAVBarrier( *histogramBuffer );

        commandList.EndEvent();
      }
      {
        commandList.BeginEvent( 0, L"Scene::Render(Calc new exposure)" );

        struct
        {
          float    targetLuminance;
          float    adaptationRate;
          float    minExposure;
          float    maxExposure;
          uint32_t pixelCount; 
        } params;

        params.targetLuminance = targetLuminance;
        params.adaptationRate  = adaptationRate;
        params.minExposure     = minExposure;
        params.maxExposure     = maxExposure;
        params.pixelCount      = lumaTexture->GetTextureWidth() * lumaTexture->GetTextureHeight();

        commandList.ChangeResourceState( *histogramBuffer, ResourceStateBits::NonPixelShaderInput );
        commandList.ChangeResourceState( *exposureBuffer, ResourceStateBits::UnorderedAccess );
        commandList.ChangeResourceState( *exposureOnlyBuffer, ResourceStateBits::UnorderedAccess );

        commandList.SetComputeShader( *adaptExposureShader );
        commandList.SetComputeConstantValues( 0, params );
        commandList.SetComputeResource( 1, *histogramBuffer->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
        commandList.SetComputeResource( 2, *exposureBuffer->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
        commandList.SetComputeResource( 3, *exposureOnlyBuffer->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
        commandList.Dispatch( 1, 1, 1 );

        commandList.AddUAVBarrier( *histogramBuffer );
        commandList.AddUAVBarrier( *exposureBuffer );
        commandList.AddUAVBarrier( *exposureOnlyBuffer );
        
        commandList.ChangeResourceState( *exposureBuffer, ResourceStateBits::NonPixelShaderInput );
        commandList.ChangeResourceState( *exposureOnlyBuffer, ResourceStateBits::NonPixelShaderInput );

        commandList.EndEvent();
      }
    }

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
    commandList.SetRenderTarget( *sdrTexture, nullptr );
    commandList.Clear( *depthTexture, 1 );

    commandList.SetPipelineState( renderManager.GetPipelinePreset( PipelinePresets::Gizmos ) );
    commandList.SetConstantValues( 0, editorInfo.selectedGizmoElement );
    gizmo->Render( commandList, XMMatrixMultiply( XMLoadFloat4x4( &editorInfo.gizmoTransform ), vpTransformNoJitter ) );

    commandList.EndEvent();
  }

  if ( editorInfo.showLuminanceHistogram )
  {
    commandList.BeginEvent( 0, L"Scene::Render(Draw histogram)" );

    commandList.AddUAVBarrier( *sdrTexture );

    commandList.ChangeResourceState( *exposureBuffer, ResourceStateBits::VertexOrConstantBuffer );
    commandList.ChangeResourceState( *histogramBuffer, ResourceStateBits::NonPixelShaderInput );
    commandList.ChangeResourceState( *sdrTexture, ResourceStateBits::UnorderedAccess );

    commandList.SetComputeShader( *debugHistogramShader );
    commandList.SetComputeConstantBuffer( 0, *exposureBuffer );
    commandList.SetComputeResource( 1, *histogramBuffer->GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
    commandList.SetComputeResource( 2, *sdrTexture->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
    commandList.Dispatch( 1, 32, 1 );

    commandList.EndEvent();
  }

  prevFrameParams = frameParams;

  for ( auto& entry : modelEntries )
    entry.second.prevNodeTransform = entry.second.nodeTransform;

  return std::pair< Resource&, Resource& >( *sdrTexture, *depthTexture );
}

void Scene::SetToneMappingParams( CommandList& commandList, bool enableAdaptation, float targetLuminance, float exposure, float adaptationRate, float minExposure, float maxExposure )
{
  this->enableAdaptation = enableAdaptation;
  this->targetLuminance  = targetLuminance;
  this->exposure         = exposure;
  this->adaptationRate   = adaptationRate;
  this->minExposure      = minExposure;
  this->maxExposure      = maxExposure;

  if ( !enableAdaptation )
    InitializeManualExposure( commandList, *exposureBuffer, *exposureOnlyBuffer, exposure );
}

void Scene::SetBloomParams( CommandList& commandList, float bloomThreshold, float bloomStrength )
{
  this->bloomThreshold = bloomThreshold;
  this->bloomStrength  = bloomStrength;
}

int Scene::GetTargetWidth() const
{
  return highResHDRTexture ? highResHDRTexture->GetTextureWidth() : lowResHDRTexture->GetTextureWidth();
}

int Scene::GetTargetHeight() const
{
  return highResHDRTexture ? highResHDRTexture->GetTextureHeight() : lowResHDRTexture->GetTextureHeight();
}

void Scene::RecreateWindowSizeDependantResources( CommandList& commandList, Window& window )
{
  RenderManager::GetInstance().IdleGPU();

  for ( auto& t : depthTextures ) t.reset();
  for ( auto& t : directLightingTextures ) t.reset();
  for ( auto& t : hqsTextures ) t.reset();
  for ( auto& t : reflectionProcTextures ) t.reset();
  for ( auto& t : bloomTextures ) for ( auto& tx : t ) tx.reset();
  highResHDRTexture.reset();
  lowResHDRTexture.reset();
  indirectLightingTexture.reset();
  reflectionTexture.reset();
  sdrTexture.reset();
  lumaTexture.reset();
  motionVectorTexture.reset();
  if ( upscaling )
  {
    upscaling->TearDown( commandList );
    upscaling.reset();
  }

  auto& device = RenderManager::GetInstance().GetDevice();

  auto width  = window.GetClientWidth();
  auto height = window.GetClientHeight();

  if ( upscalingQuality != Upscaling::Quality::Off )
  {
    upscaling = Upscaling::Instantiate();
    upscaling->Initialize( commandList, upscalingQuality, width, height );
  }

  auto lrts = upscaling ? upscaling->GetRenderingResolution() : XMINT2( width, height );

  depthTextures[ 0 ]          = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::DepthFormat, false, DepthSlot0,           std::nullopt,      false, L"DepthLQ0" );
  depthTextures[ 1 ]          = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::DepthFormat, false, DepthSlot1,           std::nullopt,      false, L"DepthLQ1" );
  directLightingTextures[ 0 ] = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::HDRFormat,   true,  DirectLighting1Slot,  std::nullopt,      false, L"Lighting1" );
  directLightingTextures[ 1 ] = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::HDRFormat,   true,  DirectLighting2Slot,  std::nullopt,      false, L"Lighting2" );
  hqsTextures[ 0 ]            = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::HQSFormat,   true,  HQS1Slot,             std::nullopt,      false, L"HQS1" );
  hqsTextures[ 1 ]            = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::HQSFormat,   true,  HQS2Slot,             std::nullopt,      false, L"HQS2" );
  indirectLightingTexture     = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::HDRFormat,   true,  IndirectLightingSlot, std::nullopt,      false, L"SpecularIBL" );
  reflectionTexture           = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::HDRFormat,   true,  ReflectionSlot,       ReflectionUAVSlot, false, L"Reflection" );
  lowResHDRTexture            = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::HDRFormat,   true,  HDRSlot,              HDRUAVSlot,        false, L"HDRLQ" );

  if ( upscaling )
  {
    highResHDRTexture   = device.Create2DTexture( commandList, width,  height, nullptr, 0, RenderManager::HDRFormat,          true,  HDRHQSlot,         HDRHQUAVSlot, false, L"HDRHQ" );
    motionVectorTexture = device.Create2DTexture( commandList, lrts.x, lrts.y, nullptr, 0, RenderManager::MotionVectorFormat, true,  MotionVectorsSlot, std::nullopt, false, L"MotionVectors" );
  }
  sdrTexture = device.Create2DTexture( commandList, width, height, nullptr, 0, RenderManager::SDRFormat, true,SDRSlot, SDRUAVSlot, false, L"SDR" );
  
  reflectionProcTextures[ 0 ] = device.Create2DTexture( commandList, lrts.x / 4, lrts.y / 4, nullptr, 0, RenderManager::HDRFormat,       false, -1, -1, false, L"ReflectionProc1" );
  reflectionProcTextures[ 1 ] = device.Create2DTexture( commandList, lrts.x / 4, lrts.y / 4, nullptr, 0, RenderManager::HDRFormat,       false, -1, -1, false, L"ReflectionProc2" );
  reflectionProcDepth         = device.Create2DTexture( commandList, lrts.x / 4, lrts.y / 4, nullptr, 0, RenderManager::ReflDepthFormat, false, -1, -1, false, L"ReflectionDepth" );

  auto bloomWidth  = width  > 2560 ? 1280 : 640;
  auto bloomHeight = height > 1440 ? 768  : 384;

  for ( int bt = 0; bt < 5; ++bt )
  {
    bloomTextures[ bt ][ 0 ] = device.Create2DTexture( commandList, bloomWidth >> bt, bloomHeight >> bt, nullptr, 0, RenderManager::HDRFormat, true, -1, -1, false, L"Bloom_a" );
    bloomTextures[ bt ][ 1 ] = device.Create2DTexture( commandList, bloomWidth >> bt, bloomHeight >> bt, nullptr, 0, RenderManager::HDRFormat, true, -1, -1, false, L"Bloom_b" );
  }

  lumaTexture = device.Create2DTexture( commandList, bloomWidth, bloomHeight, nullptr, 0, RenderManager::LumaFormat, true, -1, -1, false, L"Luma" );
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

void Scene::SetUpscalingQuality( CommandList& commandList, Window& window, Upscaling::Quality quality )
{
  if ( upscalingQuality == quality )
    return;

  upscalingQuality = quality;
  RecreateWindowSizeDependantResources( commandList, window );
}

void Scene::SetGIArea( const BoundingBox& area )
{
  giArea = area;
  giProbeInstancesDirty = true;
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

  auto giAABB = ( giArea.Extents.x < 4 || giArea.Extents.y < 4 || giArea.Extents.z < 4 ) ? sceneAABB : giArea;

  frameParams.giProbeOrigin.x = giAABB.Center.x - giAABB.Extents.x - GIProbeSpacing / 2;
  frameParams.giProbeOrigin.y = giAABB.Center.y - giAABB.Extents.y + GIProbeSpacing / 2;
  frameParams.giProbeOrigin.z = giAABB.Center.z - giAABB.Extents.z - GIProbeSpacing / 2;

  frameParams.giProbeCount.x = int( round( ( 2.0 + giAABB.Extents.x * 2 ) / GIProbeSpacing ) );
  frameParams.giProbeCount.y = int( round( ( 1.5 + giAABB.Extents.y * 2 ) / GIProbeSpacing ) );
  frameParams.giProbeCount.z = int( round( ( 2.0 + giAABB.Extents.z * 2 ) / GIProbeSpacing ) );
  frameParams.giProbeCount.w = frameParams.giProbeCount.x * frameParams.giProbeCount.z;

  std::vector< uint64_t > initGI( frameParams.giProbeCount.x * frameParams.giProbeCount.y * frameParams.giProbeCount.z, 0 );

  RenderManager::GetInstance().IdleGPU();
  giProbeTextures[ 0 ].reset();
  giProbeTextures[ 1 ].reset();

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

  commandList.BeginEvent( 0, L"Scene::UpdateRaytracing()" );

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
        rtInstances.back().accel     = usedAccel;
        rtInstances.back().transform = &entry.second.nodeTransform;
      }
    }
  }

  if ( rtState == RTState::TrianglesModified && rtScene )
    rtScene->Update( device, commandList, std::move( rtInstances ) );
  else
  {
    commandList.HoldResource( std::move( rtScene ) );
    rtScene = device.CreateRTTopLevelAccelerator( commandList, std::move( rtInstances ) );
  }

  rtState = RTState::Ready;

  commandList.EndEvent();
}

void Scene::RebuildLightCB( CommandList& commandList, const std::vector< LightCB >& lights )
{
  commandList.BeginEvent( 0, L"Scene::RebuildLightCB()" );

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

  commandList.EndEvent();
}
