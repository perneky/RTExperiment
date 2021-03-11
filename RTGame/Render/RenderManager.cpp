#include "RenderManager.h"
#include "Factory.h"
#include "Adapter.h"
#include "Device.h"
#include "Swapchain.h"
#include "PipelineState.h"
#include "Resource.h"
#include "ResourceDescriptor.h"
#include "DescriptorHeap.h"
#include "Mesh.h"
#include "ShaderStructuresNative.h"
#include "CommandQueueManager.h"
#include "CommandQueue.h"
#include "CommandAllocatorPool.h"
#include "CommandList.h"
#include "ComputeShader.h"
#include "Common/Files.h"
#include "Platform/Window.h"
#include "Game/GameDefinition.h"
#include "Game/AnimationSet.h"

RenderManager* RenderManager::instance = nullptr;

#pragma pack( push, 1 )
struct SpriteVertexFormat
{
  XMFLOAT3 position;
  XMFLOAT2 size;
  XMFLOAT2 texcoord;
  int      textureId;
};
#pragma pack( pop )

void CreateSpriteVertexDesc( VertexDesc& desc )
{
  desc.stride = sizeof( SpriteVertexFormat );

  desc.elements[ 0 ].dataType     = VertexDesc::Element::DataType::R32G32B32F;
  desc.elements[ 0 ].dataOffset   = 0;
  desc.elements[ 0 ].elementIndex = 0;
  strcpy_s( desc.elements[ 0 ].elementName, "POSITION" );

  desc.elements[ 1 ].dataType     = VertexDesc::Element::DataType::R32G32F;
  desc.elements[ 1 ].dataOffset   = 12;
  desc.elements[ 1 ].elementIndex = 0;
  strcpy_s( desc.elements[ 1 ].elementName, "SIZE" );

  desc.elements[ 2 ].dataType     = VertexDesc::Element::DataType::R32G32F;
  desc.elements[ 2 ].dataOffset   = 20;
  desc.elements[ 2 ].elementIndex = 0;
  strcpy_s( desc.elements[ 2 ].elementName, "TEXCOORD" );

  desc.elements[ 3 ].dataType     = VertexDesc::Element::DataType::R32U;
  desc.elements[ 3 ].dataOffset   = 28;
  desc.elements[ 3 ].elementIndex = 0;
  strcpy_s( desc.elements[ 3 ].elementName, "TEXID" );

  desc.elements[ 4 ].dataType = VertexDesc::Element::DataType::None;
}

void CreateRigidVertexDesc( VertexDesc& desc )
{
  desc.stride = sizeof( RigidVertexFormat );

  desc.elements[ 0 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 0 ].dataOffset   = 0;
  desc.elements[ 0 ].elementIndex = 0;
  strcpy_s( desc.elements[ 0 ].elementName, "POSITION" );

  desc.elements[ 1 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 1 ].dataOffset   = 8;
  desc.elements[ 1 ].elementIndex = 0;
  strcpy_s( desc.elements[ 1 ].elementName, "NORMAL" );

  desc.elements[ 2 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 2 ].dataOffset   = 16;
  desc.elements[ 2 ].elementIndex = 0;
  strcpy_s( desc.elements[ 2 ].elementName, "TANGENT" );

  desc.elements[ 3 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 3 ].dataOffset   = 24;
  desc.elements[ 3 ].elementIndex = 0;
  strcpy_s( desc.elements[ 3 ].elementName, "BITANGENT" );

  desc.elements[ 4 ].dataType     = VertexDesc::Element::DataType::R16G16F;
  desc.elements[ 4 ].dataOffset   = 32;
  desc.elements[ 4 ].elementIndex = 0;
  strcpy_s( desc.elements[ 4 ].elementName, "TEXCOORD" );

  desc.elements[ 5 ].dataType = VertexDesc::Element::DataType::None;
}

void CreateGIProbeVertexDesc( VertexDesc& desc )
{
  desc.stride = sizeof( GIProbeVertexFormat );

  desc.elements[ 0 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 0 ].dataOffset   = 0;
  desc.elements[ 0 ].elementIndex = 0;
  strcpy_s( desc.elements[ 0 ].elementName, "POSITION" );

  desc.elements[ 1 ].dataType = VertexDesc::Element::DataType::None;
}

void CreateGizmoVertexDesc( VertexDesc& desc )
{
  desc.stride = sizeof( GizmoVertexFormat );

  desc.elements[ 0 ].dataType     = VertexDesc::Element::DataType::R32G32B32F;
  desc.elements[ 0 ].dataOffset   = 0;
  desc.elements[ 0 ].elementIndex = 0;
  strcpy_s( desc.elements[ 0 ].elementName, "POSITION" );

  desc.elements[ 1 ].dataType     = VertexDesc::Element::DataType::R8G8B8A8UN;
  desc.elements[ 1 ].dataOffset   = 12;
  desc.elements[ 1 ].elementIndex = 0;
  strcpy_s( desc.elements[ 1 ].elementName, "COLOR" );

  desc.elements[ 2 ].dataType = VertexDesc::Element::DataType::None;
}

void CreateRigidVertexWithHistoryDesc( VertexDesc& desc )
{
  desc.stride = sizeof( RigidVertexFormatWithHistory );

  desc.elements[ 0 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 0 ].dataOffset   = 0;
  desc.elements[ 0 ].elementIndex = 0;
  strcpy_s( desc.elements[ 0 ].elementName, "POSITION" );

  desc.elements[ 1 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 1 ].dataOffset   = 8;
  desc.elements[ 1 ].elementIndex = 0;
  strcpy_s( desc.elements[ 1 ].elementName, "OLD_POSITION" );

  desc.elements[ 2 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 2 ].dataOffset   = 16;
  desc.elements[ 2 ].elementIndex = 0;
  strcpy_s( desc.elements[ 2 ].elementName, "NORMAL" );

  desc.elements[ 3 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 3 ].dataOffset   = 24;
  desc.elements[ 3 ].elementIndex = 0;
  strcpy_s( desc.elements[ 3 ].elementName, "TANGENT" );

  desc.elements[ 4 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 4 ].dataOffset   = 32;
  desc.elements[ 4 ].elementIndex = 0;
  strcpy_s( desc.elements[ 4 ].elementName, "BITANGENT" );

  desc.elements[ 5 ].dataType     = VertexDesc::Element::DataType::R16G16F;
  desc.elements[ 5 ].dataOffset   = 40;
  desc.elements[ 5 ].elementIndex = 0;
  strcpy_s( desc.elements[ 5 ].elementName, "TEXCOORD" );

  desc.elements[ 6 ].dataType = VertexDesc::Element::DataType::None;
}

void CreateSkyVertexDesc( VertexDesc& desc )
{
  desc.stride = sizeof( SkyVertexFormat );
  desc.elements[ 0 ].dataType     = VertexDesc::Element::DataType::R16G16B16A16F;
  desc.elements[ 0 ].dataOffset   = 0;
  desc.elements[ 0 ].elementIndex = 0;
  strcpy_s( desc.elements[ 0 ].elementName, "POSITION" );

  desc.elements[ 1 ].dataType     = VertexDesc::Element::DataType::None;
}

constexpr XMFLOAT2 spriteSizes[] =
{
  XMFLOAT2( 0.15f, 0.15f ),
};

bool RenderManager::CreateInstance( std::shared_ptr< Window > window )
{
  instance = new RenderManager( window );
  return true;
}

RenderManager& RenderManager::GetInstance()
{
  assert( instance );
  return *instance;
}

void RenderManager::DeleteInstance()
{
  if ( instance )
    delete instance;
  instance = nullptr;
}

void RenderManager::IdleGPU()
{
  commandQueueManager->IdleGPU();
}

CommandAllocator* RenderManager::RequestCommandAllocator( CommandQueueType queueType )
{
  auto& queue          = commandQueueManager->GetQueue( queueType );
  auto& allocatorPool  = commandQueueManager->GetAllocatorPool( queueType );
  return allocatorPool.RequestAllocator( *device, queue.GetLastCompletedFenceValue() );
}

void RenderManager::DiscardCommandAllocator( CommandQueueType queueType, CommandAllocator* allocator, uint64_t fenceValue )
{
  commandQueueManager->GetAllocatorPool( queueType ).DiscardAllocator( fenceValue, allocator );
}

std::unique_ptr< CommandList > RenderManager::CreateCommandList( CommandAllocator* allocator, CommandQueueType queueType )
{
  auto commandList = device->CreateCommandList( *allocator, queueType );
  commandList->BeginEvent( 0, L"CommandList" );
  return commandList;
}

void RenderManager::PrepareNextSwapchainBuffer( uint64_t fenceValue )
{
  commandQueueManager->GetQueue( CommandQueueType::Direct ).WaitForFence( fenceValue );
}

uint64_t RenderManager::Submit( std::unique_ptr< CommandList > commandList, CommandQueueType queueType, bool wait )
{
  commandList->EndEvent();

  auto& queue      = commandQueueManager->GetQueue( queueType );
  auto  fenceValue = queue.Submit( *commandList );
  if ( wait )
    queue.WaitForFence( fenceValue );
  else
  {
    auto heldResources = commandList->TakeHeldResources();
    if ( !heldResources.empty() )
    {
      auto& hold = stagingResources[ queueType ][ fenceValue ];
      std::move( heldResources.begin(), heldResources.end(), std::back_inserter( hold ) );
    }
  }
  return fenceValue;
}

uint64_t RenderManager::Present( uint64_t fenceValue )
{
  return swapchain->Present( fenceValue );
}

int RenderManager::ReserveRTIndexBufferSlot()
{
  std::lock_guard< std::mutex > lock( freeRTIndexBufferSlotsLock );
  assert( !freeRTIndexBufferSlots.empty() );
  int slot = freeRTIndexBufferSlots.back();
  freeRTIndexBufferSlots.pop_back();
  return slot;
}

void RenderManager::FreeRTIndexBufferSlot( int slot )
{
  std::lock_guard< std::mutex > lock( freeRTIndexBufferSlotsLock );
  freeRTIndexBufferSlots.emplace_back( slot );
}

int RenderManager::ReserveRTVertexBufferSlot()
{
  std::lock_guard< std::mutex > lock( freeRTVertexBufferSlotsLock );
  assert( !freeRTVertexBufferSlots.empty() );
  int slot = freeRTVertexBufferSlots.back();
  freeRTVertexBufferSlots.pop_back();
  return slot;
}

void RenderManager::FreeRTVertexBufferSlot( int slot )
{
  std::lock_guard< std::mutex > lock( freeRTVertexBufferSlotsLock );
  freeRTVertexBufferSlots.emplace_back( slot );
}

PipelineState& RenderManager::GetPipelinePreset( PipelinePresets preset )
{
  return *pipelinePresets[ int( preset ) ];
}

DescriptorHeap& RenderManager::GetShaderResourceHeap()
{
  return device->GetShaderResourceHeap();
}

void RenderManager::PrepareAllMaterials( CommandList& commandList )
{
  assert( gameDefinition.GetEntityMaterials().size() < MaxMaterials );

  MaterialCB allMaterials[ MaxMaterials ];
  auto iter = 0;
  for ( auto& material : gameDefinition.GetEntityMaterials() )
  {
    int albedoIx    = material.second.colorMapPath    .empty() ? -1 : Load2DTexture( commandList, material.second.colorMapPath     );
    int normalIx    = material.second.normalMapPath   .empty() ? -1 : Load2DTexture( commandList, material.second.normalMapPath    );
    int metallicIx  = material.second.metallicMapPath .empty() ? -1 : Load2DTexture( commandList, material.second.metallicMapPath  );
    int roughnessIx = material.second.roughnessMapPath.empty() ? -1 : Load2DTexture( commandList, material.second.roughnessMapPath );
    int emissiveIx  = material.second.emissiveMapPath .empty() ? -1 : Load2DTexture( commandList, material.second.emissiveMapPath  );

    auto& cbMaterial = allMaterials[ iter ];
    cbMaterial.textureIndices = XMINT4( albedoIx, normalIx, metallicIx, roughnessIx );
    cbMaterial.args.x        = 0;
    cbMaterial.args.x       |= material.second.flipNormalX ? MaterialFeatures::FlipNormalX : 0;
    cbMaterial.args.x       |= material.second.scaleuv     ? MaterialFeatures::ScaleUV     : 0;
    cbMaterial.args.x       |= material.second.clampToEdge ? MaterialFeatures::ClampToEdge : 0;
    cbMaterial.args.x       |= material.second.useWetness  ? MaterialFeatures::UseWetness  : 0;
    cbMaterial.args.x       |= material.second.twoSided    ? MaterialFeatures::TwoSided    : 0;
    cbMaterial.args.x       |= material.second.useSpecular ? MaterialFeatures::UseSpecular : 0;
    cbMaterial.args.y        = emissiveIx;
    cbMaterial.args.w        = int( ConvertToAlphaMode( material.second.blendMode ) );
    cbMaterial.textureValues = XMFLOAT4( material.second.metallicValue, material.second.roughnessValue, 0, 0 );

    allMaterialMap[ material.first ] = iter;

    ++iter;
  }

  for ( auto& material : gameDefinition.GetSkyMaterials() )
  {
    int albedoIx = material.second.colorMapPath .empty() ? -1 : LoadCubeTexture( commandList, material.second.colorMapPath  );
    int normalIx = material.second.normalMapPath.empty() ? -1 : LoadCubeTexture( commandList, material.second.normalMapPath );

    auto& cbMaterial = allMaterials[ iter ];
    cbMaterial.textureIndices = XMINT4( albedoIx - CubeTextureBaseSlot, normalIx - CubeTextureBaseSlot, -1, -1 );

    allMaterialMap[ material.first ] = iter;

    ++iter;
  }

  allMaterialsConstantBuffer = device->CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( allMaterials ), sizeof( MaterialCB ), L"AllMaterials" );
  auto ub = RenderManager::GetInstance().GetDevice().CreateBuffer( ResourceType::ConstantBuffer, HeapType::Upload, false, sizeof( allMaterials ), sizeof( MaterialCB ), L"AllMaterialsUpload" );
  commandList.UploadBufferResource( std::move( ub ), *allMaterialsConstantBuffer, &allMaterials, sizeof( allMaterials ) );

  spriteTextures[ int( Sprites::Light ) ] = Load2DTexture( commandList, L"Content\\Textures\\LampIcon.dds" );

  struct NoiseTexel
  {
    unsigned r : 10;
    unsigned g : 10;
    unsigned b : 10;
    unsigned a : 2;
  };

  srand( 68425 );
  constexpr int noiseSize = RandomTextureSize;
  std::vector< NoiseTexel > noiseTexels( noiseSize * noiseSize * noiseSize );
  for ( auto& t : noiseTexels )
  {
    t.r = rand() % ( 1 << 10 );
    t.g = rand() % ( 1 << 10 );
    t.b = rand() % ( 1 << 10 );
    t.a = 4;
  }

  randomTexture = device->CreateVolumeTexture( commandList, noiseSize, noiseSize, noiseSize, noiseTexels.data(), int( noiseTexels.size() * 4 ), PixelFormat::RGBA1010102UN, RandomTextureSlotCPP, std::nullopt, L"NoiseVolume" );

  auto skinTransformShaderFile = ReadFileToMemory( L"Content/Shaders/SkinTransform.cso" );
  skinTransformShader = device->CreateComputeShader( skinTransformShaderFile.data(), int( skinTransformShaderFile.size() ), L"SkinTransform" );
}

int RenderManager::GetAllMaterialIndex( const GUID& guid ) const
{
  auto iter = allMaterialMap.find( guid );
  assert( iter != allMaterialMap.end() );
  return iter->second;
}

void RenderManager::BindAllMaterials( CommandList& commandList, int slot )
{
  commandList.SetConstantBuffer( slot, *allMaterialsConstantBuffer );
}

void RenderManager::BindAllMaterialsToCompute( CommandList& commandList, int slot )
{
  commandList.SetComputeConstantBuffer( slot, *allMaterialsConstantBuffer );
}

ComputeShader& RenderManager::GetSkinTransformShader()
{
  return *skinTransformShader;
}

Resource& RenderManager::GetSkinTransformBuffer()
{
  return *skinTransformBuffer;
}

void RenderManager::TidyUp()
{
  for ( int queueType = 0; queueType < 3; ++queueType )
  {
    auto& queue     = commandQueueManager->GetQueue( CommandQueueType( queueType ) );
    auto& resources = stagingResources[ CommandQueueType( queueType ) ];
    for ( auto iter = resources.begin(); iter != resources.end(); )
    {
      if ( queue.IsFenceComplete( iter->first ) )
      {
        for ( auto& resource : iter->second )
        {
          if ( resource->IsUploadResource() && resource->GetResourceType() == ResourceType::ConstantBuffer )
            ReuseUploadConstantBuffer( std::move( resource ) );
        }
        iter = resources.erase( iter );
      }
      else
        ++iter;
    }
  }
}

std::unique_ptr< Resource > RenderManager::GetUploadConstantBufferForResource( Resource& resource )
{
  auto size = device->GetUploadSizeForResource( resource );

  std::lock_guard< std::mutex > lock( uploadConstantBuffersLock );

  auto iter = uploadConstantBuffers.find( size );
  if ( iter == uploadConstantBuffers.end() || iter->second.empty() )
    return device->CreateBuffer( ResourceType::ConstantBuffer, HeapType::Upload, false, size, 0, L"UploadCB" );
  else
  {
    auto buffer = std::move( iter->second.back() );
    iter->second.pop_back();
    return buffer;
  }
}

void RenderManager::ReuseUploadConstantBuffer( std::unique_ptr< Resource > buffer )
{
  auto bufferSize = buffer->GetBufferSize();
  uploadConstantBuffers[ bufferSize ].emplace_back( std::move( buffer ) );
}

Device& RenderManager::GetDevice()
{
  return *device;
}

Swapchain& RenderManager::GetSwapchain()
{
  return *swapchain;
}

RenderManager::RenderManager( std::shared_ptr< Window > window )
  : window( window )
{
  assert( EngineTextureBaseSlot   == atoi( EngineTextureBaseSlotStr   ) );
  assert( MaterialTextureBaseSlot == atoi( MaterialTextureBaseSlotStr ) );
  assert( CubeTextureBaseSlot     == atoi( CubeTextureBaseSlotStr     ) );
  assert( VolTextureBaseSlot      == atoi( VolTextureBaseSlotStr      ) );
  assert( EngineResourceCount     == atoi( EngineTextureCountStr      ) );
  assert( MaterialTextureCount    == atoi( MaterialTextureCountStr    ) );
  assert( CubeTextureCount        == atoi( CubeTextureCountStr        ) );
  assert( VolTextureCount         == atoi( VolTextureCountStr         ) );
  assert( CBVIBBaseSlot           == atoi( CBVIBBaseSlotStr           ) );
  assert( CBVVBBaseSlot           == atoi( CBVVBBaseSlotStr           ) );

  freeRTIndexBufferSlots.reserve( MaxMeshCount );
  for ( int slot = 0; slot < MaxMeshCount; ++slot )
    freeRTIndexBufferSlots.emplace_back( CBVIBBaseSlot + slot );

  freeRTVertexBufferSlots.reserve( MaxMeshCount );
  for ( int slot = 0; slot < MaxMeshCount; ++slot )
    freeRTVertexBufferSlots.emplace_back( CBVVBBaseSlot + slot );

  factory = Factory::Create();
  adapter = factory->CreateDefaultAdapter();
  device  = adapter->CreateDevice();

  commandQueueManager.reset( new CommandQueueManager( *device ) );

  swapchain = factory->CreateSwapchain( *device, commandQueueManager->GetQueue( CommandQueueType::Direct ), *window );
  swapchain->BuildBackBufferTextures( *device );

  spriteVertexBuffer  = device->CreateBuffer( ResourceType::VertexBuffer,   HeapType::Default, false, MaxSpriteVertices * sizeof( SpriteVertexFormat ), sizeof( SpriteVertexFormat ), L"SpriteVertexBuffer" );
  skinTransformBuffer = device->CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, 256 * sizeof( XMFLOAT4X4 ), sizeof( XMFLOAT4X4 ), L"SkinTransformBuffer" );

  const int oneBitAlphaDepthBias = 0;// -1000;

  {
    auto vertexShader = ReadFileToMemory( L"Content/Shaders/DepthVS.cso" );
    auto  pixelShader = ReadFileToMemory( L"Content/Shaders/DepthPS.cso" );

    PipelineDesc pipelineDesc;

    CreateRigidVertexDesc( pipelineDesc.vertexDesc );
    pipelineDesc.rasterizerDesc.cullFront = RasterizerDesc::CullFront::Clockwise;
    pipelineDesc.blendDesc                = BlendDesc( BlendPreset::DisableColorWrite );
    pipelineDesc.vsData                   = vertexShader.data();
    pipelineDesc.psData                   = pixelShader.data();
    pipelineDesc.vsSize                   = vertexShader.size();
    pipelineDesc.psSize                   = pixelShader.size();
    pipelineDesc.primitiveType            = PrimitiveType::TriangleList;
    pipelineDesc.depthFormat              = DepthFormat;
    pipelineDesc.samples                  = 1;
    pipelinePresets[ int( PipelinePresets::Depth ) ] = device->CreatePipelineState( pipelineDesc );

    pipelineDesc.rasterizerDesc.cullMode  = RasterizerDesc::CullMode::None;
    pipelineDesc.rasterizerDesc.depthBias = oneBitAlphaDepthBias;
    pipelinePresets[ int( PipelinePresets::DepthOneBitAlpha ) ] = device->CreatePipelineState( pipelineDesc );

    CreateRigidVertexWithHistoryDesc( pipelineDesc.vertexDesc );

    pipelineDesc.rasterizerDesc.cullMode  = RasterizerDesc::CullMode::Back;
    pipelineDesc.rasterizerDesc.depthBias = 0;
    pipelinePresets[ int( PipelinePresets::DepthWithHistory ) ] = device->CreatePipelineState( pipelineDesc );

    pipelineDesc.rasterizerDesc.cullMode  = RasterizerDesc::CullMode::None;
    pipelineDesc.rasterizerDesc.depthBias = oneBitAlphaDepthBias;
    pipelinePresets[ int( PipelinePresets::DepthOneBitAlphaWithHistory ) ] = device->CreatePipelineState( pipelineDesc );
  }

  {
    auto vertexShader = ReadFileToMemory( L"Content/Shaders/ModelVS.cso" );
    auto  pixelShader = ReadFileToMemory( L"Content/Shaders/ModelPS.cso" );

    PipelineDesc pipelineDesc;

    CreateRigidVertexDesc( pipelineDesc.vertexDesc );
    pipelineDesc.rasterizerDesc.cullFront    = RasterizerDesc::CullFront::Clockwise;
    pipelineDesc.blendDesc                   = BlendDesc( BlendPreset::DirectWrite );
    pipelineDesc.vsData                      = vertexShader.data();
    pipelineDesc.psData                      = pixelShader.data();
    pipelineDesc.vsSize                      = vertexShader.size();
    pipelineDesc.psSize                      = pixelShader.size();
    pipelineDesc.primitiveType               = PrimitiveType::TriangleList;
    pipelineDesc.targetFormat[ 0 ]           = RenderManager::HDRFormat;
    pipelineDesc.targetFormat[ 1 ]           = RenderManager::HDRFormat;
    pipelineDesc.targetFormat[ 2 ]           = RenderManager::HDRFormat;
    pipelineDesc.depthFormat                 = RenderManager::DepthFormat;
    pipelineDesc.samples                     = 1;
    pipelineDesc.depthStencilDesc.depthWrite = false;
    pipelinePresets[ int( PipelinePresets::Mesh ) ] = device->CreatePipelineState( pipelineDesc );

    pipelineDesc.rasterizerDesc.cullMode  = RasterizerDesc::CullMode::None;
    pipelineDesc.rasterizerDesc.depthBias = oneBitAlphaDepthBias;
    pipelinePresets[ int( PipelinePresets::MeshOneBitAlpha ) ] = device->CreatePipelineState( pipelineDesc );
  }

  {
    auto vertexShader = ReadFileToMemory( L"Content/Shaders/ModelWithHistoryVS.cso" );
    auto  pixelShader = ReadFileToMemory( L"Content/Shaders/ModelPS.cso" );

    PipelineDesc pipelineDesc;

    CreateRigidVertexWithHistoryDesc( pipelineDesc.vertexDesc );
    pipelineDesc.rasterizerDesc.cullFront    = RasterizerDesc::CullFront::Clockwise;
    pipelineDesc.blendDesc                   = BlendDesc( BlendPreset::DirectWrite );
    pipelineDesc.vsData                      = vertexShader.data();
    pipelineDesc.psData                      = pixelShader.data();
    pipelineDesc.vsSize                      = vertexShader.size();
    pipelineDesc.psSize                      = pixelShader.size();
    pipelineDesc.primitiveType               = PrimitiveType::TriangleList;
    pipelineDesc.targetFormat[ 0 ]           = RenderManager::HDRFormat;
    pipelineDesc.targetFormat[ 1 ]           = RenderManager::HDRFormat;
    pipelineDesc.targetFormat[ 2 ]           = RenderManager::HDRFormat;
    pipelineDesc.depthFormat                 = RenderManager::DepthFormat;
    pipelineDesc.samples                     = 1; 
    pipelineDesc.depthStencilDesc.depthWrite = false;
    pipelinePresets[ int( PipelinePresets::MeshWithHistory ) ] = device->CreatePipelineState( pipelineDesc );

    pipelineDesc.rasterizerDesc.cullMode  = RasterizerDesc::CullMode::None;
    pipelineDesc.rasterizerDesc.depthBias = oneBitAlphaDepthBias;
    pipelinePresets[ int( PipelinePresets::MeshOneBitAlphaWithHistory ) ] = device->CreatePipelineState( pipelineDesc );
  }

  {
    auto vertexShader = ReadFileToMemory( L"Content/Shaders/ModelVS.cso" );
    auto  pixelShader = ReadFileToMemory( L"Content/Shaders/TranslucentModelPS.cso" );

    PipelineDesc pipelineDesc;

    CreateRigidVertexDesc( pipelineDesc.vertexDesc );
    pipelineDesc.depthStencilDesc.depthWrite = false;
    pipelineDesc.rasterizerDesc.cullFront    = RasterizerDesc::CullFront::Clockwise;
    pipelineDesc.blendDesc                   = BlendDesc( BlendPreset::Blend );
    pipelineDesc.vsData                      = vertexShader.data();
    pipelineDesc.psData                      = pixelShader.data();
    pipelineDesc.vsSize                      = vertexShader.size();
    pipelineDesc.psSize                      = pixelShader.size();
    pipelineDesc.primitiveType               = PrimitiveType::TriangleList;
    pipelineDesc.targetFormat[ 0 ]           = RenderManager::HDRFormat;
    pipelineDesc.depthFormat                 = RenderManager::DepthFormat;
    pipelineDesc.samples                     = 1;
    pipelinePresets[ int( PipelinePresets::MeshTranslucent ) ] = device->CreatePipelineState( pipelineDesc );

    pipelineDesc.rasterizerDesc.cullMode = RasterizerDesc::CullMode::None;
    pipelinePresets[ int( PipelinePresets::MeshTranslucentTwoSided ) ] = device->CreatePipelineState( pipelineDesc );
  }

  {
    auto vertexShader = ReadFileToMemory( L"Content/Shaders/SkyboxVS.cso" );
    auto  pixelShader = ReadFileToMemory( L"Content/Shaders/SkyboxPS.cso" );

    PipelineDesc pipelineDesc;

    CreateSkyVertexDesc( pipelineDesc.vertexDesc );
    pipelineDesc.rasterizerDesc.cullMode  = RasterizerDesc::CullMode::None;
    pipelineDesc.blendDesc                = BlendDesc( BlendPreset::DirectWrite );
    pipelineDesc.vsData                   = vertexShader.data();
    pipelineDesc.psData                   = pixelShader.data();
    pipelineDesc.vsSize                   = vertexShader.size();
    pipelineDesc.psSize                   = pixelShader.size();
    pipelineDesc.primitiveType            = PrimitiveType::TriangleStrip;
    pipelineDesc.targetFormat[ 0 ]        = RenderManager::HDRFormat;
    pipelineDesc.depthFormat              = RenderManager::DepthFormat;
    pipelineDesc.samples                  = 1;
    pipelinePresets[ int( PipelinePresets::Skybox ) ] = device->CreatePipelineState( pipelineDesc );
  }

  {
    auto vertexShader = ReadFileToMemory( L"Content/Shaders/SpriteVS.cso" );
    auto  pixelShader = ReadFileToMemory( L"Content/Shaders/SpritePS.cso" );

    PipelineDesc pipelineDesc;

    CreateSpriteVertexDesc( pipelineDesc.vertexDesc );
    pipelineDesc.depthStencilDesc.depthWrite = false;
    pipelineDesc.rasterizerDesc.cullMode     = RasterizerDesc::CullMode::None;
    pipelineDesc.blendDesc                   = BlendDesc( BlendPreset::Blend );
    pipelineDesc.vsData                      = vertexShader.data();
    pipelineDesc.psData                      = pixelShader.data();
    pipelineDesc.vsSize                      = vertexShader.size();
    pipelineDesc.psSize                      = pixelShader.size();
    pipelineDesc.primitiveType               = PrimitiveType::TriangleList;
    pipelineDesc.targetFormat[ 0 ]           = RenderManager::SDRFormat;
    pipelineDesc.depthFormat                 = RenderManager::DepthFormat;
    pipelineDesc.samples                     = 1;
    pipelinePresets[ int( PipelinePresets::Sprite ) ] = device->CreatePipelineState( pipelineDesc );
  }

  {
    auto vertexShader = ReadFileToMemory( L"Content/Shaders/GIProbeVS.cso" );
    auto  pixelShader = ReadFileToMemory( L"Content/Shaders/GIProbePS.cso" );

    PipelineDesc pipelineDesc;

    CreateGIProbeVertexDesc( pipelineDesc.vertexDesc );
    pipelineDesc.rasterizerDesc.cullMode     = RasterizerDesc::CullMode::None;
    pipelineDesc.blendDesc                   = BlendDesc( BlendPreset::DirectWrite );
    pipelineDesc.vsData                      = vertexShader.data();
    pipelineDesc.psData                      = pixelShader.data();
    pipelineDesc.vsSize                      = vertexShader.size();
    pipelineDesc.psSize                      = pixelShader.size();
    pipelineDesc.primitiveType               = PrimitiveType::TriangleList;
    pipelineDesc.targetFormat[ 0 ]           = RenderManager::HDRFormat;
    pipelineDesc.depthFormat                 = RenderManager::DepthFormat;
    pipelineDesc.samples                     = 1;
    pipelinePresets[ int( PipelinePresets::GIProbes ) ] = device->CreatePipelineState( pipelineDesc );
  }

  {
    auto vertexShader = ReadFileToMemory( L"Content/Shaders/GizmoVS.cso" );
    auto  pixelShader = ReadFileToMemory( L"Content/Shaders/GizmoPS.cso" );

    PipelineDesc pipelineDesc;

    CreateGizmoVertexDesc( pipelineDesc.vertexDesc );
    pipelineDesc.rasterizerDesc.cullMode     = RasterizerDesc::CullMode::None;
    pipelineDesc.blendDesc                   = BlendDesc( BlendPreset::Blend );
    pipelineDesc.vsData                      = vertexShader.data();
    pipelineDesc.psData                      = pixelShader.data();
    pipelineDesc.vsSize                      = vertexShader.size();
    pipelineDesc.psSize                      = pixelShader.size();
    pipelineDesc.primitiveType               = PrimitiveType::TriangleList;
    pipelineDesc.targetFormat[ 0 ]           = RenderManager::SDRFormat;
    pipelineDesc.depthFormat                 = RenderManager::DepthFormat;
    pipelineDesc.samples                     = 1;
    pipelinePresets[ int( PipelinePresets::Gizmos ) ] = device->CreatePipelineState( pipelineDesc );
  }
}

RenderManager::~RenderManager()
{
  // We need to destruct all objects in the correct order!

  commandQueueManager->IdleGPU();
  stagingResources.clear();
  uploadConstantBuffers.clear();
  allMaterialsConstantBuffer.reset();
  randomTexture.reset();
  skinTransformBuffer.reset();
  skinTransformShader.reset();
  spriteVertexBuffer.reset();
  for ( auto& tex : allTextures )
    tex.reset();

  commandQueueManager.reset();

  for ( auto& preset : pipelinePresets )
    preset.reset();

  swapchain.reset();
  device.reset();
  adapter.reset();
  factory.reset();
  window.reset();
}

void RenderManager::RecreateWindowSizeDependantResources( CommandList& commandList )
{
}

void RenderManager::BeginSpriteRendering( CommandList& commandList )
{
  commandList.BeginEvent( 0, L"RenderManager::SpriteRendering()" );
  spriteVertices.reserve( MaxSpriteVertices );
}

void RenderManager::FinishSpriteRendering( CommandList& commandList, FXMMATRIX viewTransform, CXMMATRIX projTransform )
{
  if ( !spriteVertices.empty() )
  {
    XMMATRIX billboard = viewTransform;
    billboard.r[ 0 ] = XMVectorSet( 1, 0, 0, 0 );
    billboard.r[ 1 ] = XMVectorSet( 0, 1, 0, 0 );
    billboard.r[ 2 ] = XMVectorSet( 0, 0, 1, 0 );

    XMFLOAT4X4 t[ 2 ];
    XMStoreFloat4x4( &t[ 0 ], viewTransform );
    XMStoreFloat4x4( &t[ 1 ], projTransform );

    commandList.SetPipelineState( GetPipelinePreset( PipelinePresets::Sprite ) );
    commandList.SetConstantValues( 0, t );
    commandList.SetTextureHeap( 1, GetShaderResourceHeap(), 0 );

    int  dataSize = int( spriteVertices.size() * sizeof( SpriteVertexFormat ) );
    auto ub       = this->GetUploadConstantBufferForResource( *spriteVertexBuffer );
    commandList.UploadBufferResource( std::move( ub ), *spriteVertexBuffer, spriteVertices.data(), dataSize );
    commandList.SetVertexBuffer( *spriteVertexBuffer );

    commandList.Draw( int( spriteVertices.size() ) );

    spriteVertices.clear();
  }

  commandList.EndEvent();
}

void RenderManager::AddSprite( CommandList& commandList, FXMVECTOR position, Sprites type )
{
  auto& size = spriteSizes[ int( type ) ];

  SpriteVertexFormat v;
  XMStoreFloat3( &v.position, position );
  v.textureId = spriteTextures[ int( type ) ];

  v.size.x     = - size.x;
  v.size.y     = + size.y;
  v.texcoord.x = 0;
  v.texcoord.y = 0;
  spriteVertices.emplace_back( v );

  v.size.x     = + size.x;
  v.size.y     = + size.y;
  v.texcoord.x = 1;
  v.texcoord.y = 0;
  spriteVertices.emplace_back( v );

  v.size.x     = - size.x;
  v.size.y     = - size.y;
  v.texcoord.x = 0;
  v.texcoord.y = 1;
  spriteVertices.emplace_back( v );

  //--------------

  v.size.x     = + size.x;
  v.size.y     = + size.y;
  v.texcoord.x = 1;
  v.texcoord.y = 0;
  spriteVertices.emplace_back( v );

  v.size.x     = + size.x;
  v.size.y     = - size.y;
  v.texcoord.x = 1;
  v.texcoord.y = 1;
  spriteVertices.emplace_back( v );

  v.size.x     = - size.x;
  v.size.y     = - size.y;
  v.texcoord.x = 0;
  v.texcoord.y = 1;
  spriteVertices.emplace_back( v );

  assert( spriteVertices.size() < MaxSpriteVertices );
}

int RenderManager::Load2DTexture( CommandList& commandList, const std::wstring& path )
{
  auto iter = allTextureMap.find( path );
  if ( iter != allTextureMap.end() )
    return iter->second;

  int slot = DoLoad2DTexture( commandList, path );
  if ( slot >= 0 )
    allTextureMap[ path ] = slot;
  return slot;
}

int RenderManager::DoLoad2DTexture( CommandList& commandList, const std::wstring& path )
{
  auto fileData = ReadFileToMemory( path.data() );
  if ( fileData.empty() )
    return -1;

  int slot = 0;
  while ( allTextures[ slot ] )
    slot++;

  assert( slot < MaterialTextureCount );

  allTextures[ slot ] = device->Load2DTexture( commandList, std::move( fileData ), MaterialTextureBaseSlot + slot, path.data() );

  return slot;
}

int RenderManager::LoadCubeTexture( CommandList& commandList, const std::wstring& path )
{
  auto iter = allTextureMap.find( path );
  if ( iter != allTextureMap.end() )
    return iter->second;

  int slot = DoLoadCubeTexture( commandList, path );
  if ( slot >= 0 )
    allTextureMap[ path ] = slot;
  return slot;
}

int RenderManager::DoLoadCubeTexture( CommandList& commandList, const std::wstring& path )
{
  auto fileData = ReadFileToMemory( path.data() );
  if ( fileData.empty() )
    return -1;

  int slot = CubeTextureBaseSlot;
  while ( allTextures[ slot ] )
    slot++;

  assert( slot < CubeTextureBaseSlot + CubeTextureCount );

  allTextures[ slot ] = device->LoadCubeTexture( commandList, std::move( fileData ), slot, path.data() );

  return slot;
}

std::shared_ptr< AnimationSet > RenderManager::LoadAnimation( const wchar_t* path, std::function< void( AnimationSet* ) > filler )
{
  std::lock_guard< std::mutex > lock( animationMapLock );

  auto iter = animationMap.find( path );
  if ( iter != animationMap.end() )
    if ( auto locked = iter->second.lock() )
      return locked;

  auto animFile = ReadFileToMemory( path );
  auto animSet  = std::make_shared< AnimationSet >( std::move( animFile ) );
  filler( animSet.get() );
  animationMap[ path ] = animSet;
  return animSet;
}

