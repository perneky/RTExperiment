#include "D3DDevice.h"
#include "D3DAdapter.h"
#include "D3DCommandQueue.h"
#include "D3DCommandAllocator.h"
#include "D3DCommandList.h"
#include "D3DPipelineState.h"
#include "D3DResource.h"
#include "D3DResourceDescriptor.h"
#include "D3DDescriptorHeap.h"
#include "D3DRTBottomLevelAccelerator.h"
#include "D3DRTTopLevelAccelerator.h"
#include "D3DComputeShader.h"
#include "D3DUtils.h"
#include "Conversion.h"
#include "DirectXTex/DDSTextureLoader/DDSTextureLoader12.h"
#include "Common/Files.h"
#include "../ShaderValues.h"
#include "../DearImGui/imgui_impl_dx12.h"

struct D3DDeviceHelper
{
  static void FillTexture( D3DCommandList& commandList, D3DDevice& d3dDevice, D3DResource& d3dResource, const D3D12_SUBRESOURCE_DATA* subresources, int numSubresources, int slot )
  {
    auto resourceDesc = d3dResource.GetD3DResource()->GetDesc();

    UINT64 intermediateSize;
    d3dDevice.GetD3DDevice()->GetCopyableFootprints( &resourceDesc, 0, numSubresources, 0, nullptr, nullptr, nullptr, &intermediateSize );

    CComPtr< ID3D12Resource > d3dUploadResource = AllocateUploadBuffer( d3dDevice.GetD3DDevice(), intermediateSize );

    auto oldState = d3dResource.GetCurrentResourceState();
    commandList.ChangeResourceState( d3dResource, ResourceStateBits::CopyDestination );

    auto uploadResult = UpdateSubresources( static_cast< D3DCommandList* >( &commandList )->GetD3DGraphicsCommandList()
                                          , d3dResource.GetD3DResource()
                                          , d3dUploadResource
                                          , 0
                                          , 0
                                          , numSubresources
                                          , subresources );

    commandList.ChangeResourceState( d3dResource, oldState );

    assert( uploadResult != 0 );

    commandList.ChangeResourceState( d3dResource, ResourceStateBits::NonPixelShaderInput | ResourceStateBits::PixelShaderInput );

    CComPtr< ID3D12Resource2 > d3dUploadResource2;
    d3dUploadResource.QueryInterface( &d3dUploadResource2 );
    commandList.HoldResource( std::unique_ptr< Resource >( new D3DResource( d3dUploadResource2, ResourceStateBits::Common ) ) );
  }
};

constexpr int mipmapGenHeapSize = 200;

D3DDevice::D3DDevice( D3DAdapter& adapter )
{
  if FAILED( D3D12CreateDevice( adapter.GetDXGIAdapter(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS( &d3dDevice ) ) )
    return;

#if DEBUG_GFX_API
  CComPtr< ID3D12InfoQueue > infoQueue;
  if SUCCEEDED( d3dDevice->QueryInterface( &infoQueue ) )
  {
    infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
    infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR,      TRUE );
    infoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING,    TRUE );

    D3D12_MESSAGE_SEVERITY severities[] =
    {
        D3D12_MESSAGE_SEVERITY_INFO
    };

    D3D12_MESSAGE_ID denyIds[] = 
    {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
        D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
    };

    D3D12_INFO_QUEUE_FILTER newFilter = {};
    newFilter.DenyList.NumCategories = 0;
    newFilter.DenyList.pCategoryList = nullptr;
    newFilter.DenyList.NumSeverities = _countof( severities );
    newFilter.DenyList.pSeverityList = severities;
    newFilter.DenyList.NumIDs        = _countof( denyIds );
    newFilter.DenyList.pIDList       = denyIds;

    infoQueue->PushStorageFilter( &newFilter );
  }
#endif // DEBUG_GFX_API

  descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ].reset( new D3DDescriptorHeap( *this, VaryingResourceBaseSlot, CBVHeapSize, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ) );
  descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER     ].reset( new D3DDescriptorHeap( *this, 0, 20, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ) );
  descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_RTV         ].reset( new D3DDescriptorHeap( *this, 0, 20, D3D12_DESCRIPTOR_HEAP_TYPE_RTV     ) );
  descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_DSV         ].reset( new D3DDescriptorHeap( *this, 0, 20, D3D12_DESCRIPTOR_HEAP_TYPE_DSV     ) );

  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors = 100;
  desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  d3dDevice->CreateDescriptorHeap( &desc, IID_PPV_ARGS( &d3dDearImGuiHeap ) );

  ImGui_ImplDX12_Init( d3dDevice
                     , 3
                     , DXGI_FORMAT_R10G10B10A2_UNORM
                     , d3dDearImGuiHeap
                     , d3dDearImGuiHeap->GetCPUDescriptorHandleForHeapStart()
                     , d3dDearImGuiHeap->GetGPUDescriptorHandleForHeapStart() );

  SetContainerObject( d3dDevice, this );

  auto shaderData = ReadFileToMemory( L"Content/Shaders/Downsample.cso" );
  mipmapGenComputeShader.reset( new D3DComputeShader( *this, shaderData.data(), int( shaderData.size() ), L"MipMapGen" ) );

  D3D12_DESCRIPTOR_HEAP_DESC mipmapGenHeapDesc = {};
  mipmapGenHeapDesc.NumDescriptors = mipmapGenHeapSize;
  mipmapGenHeapDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  mipmapGenHeapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  d3dDevice->CreateDescriptorHeap( &mipmapGenHeapDesc, IID_PPV_ARGS( &d3dmipmapGenHeap ) );

  mipmapGenDescCounter = 0;
}

D3DDevice::~D3DDevice()
{
  ImGui_ImplDX12_Shutdown();
}

std::unique_ptr< CommandQueue > D3DDevice::CreateCommandQueue( CommandQueueType type )
{
  return std::unique_ptr< CommandQueue >( new D3DCommandQueue( *this, type ) );
}

std::unique_ptr< CommandAllocator > D3DDevice::CreateCommandAllocator( CommandQueueType type )
{
  return std::unique_ptr< CommandAllocator >( new D3DCommandAllocator( *this, type ) );
}

std::unique_ptr< CommandList > D3DDevice::CreateCommandList( CommandAllocator& commandAllocator, CommandQueueType queueType )
{
  return std::unique_ptr< CommandList >( new D3DCommandList( *this, *static_cast< D3DCommandAllocator* >( &commandAllocator ), queueType ) );
}

std::unique_ptr< PipelineState > D3DDevice::CreatePipelineState( const PipelineDesc& desc )
{
  return std::unique_ptr< PipelineState >( new D3DPipelineState( desc, *this ) );
}

std::unique_ptr< Resource > D3DDevice::CreateBuffer( ResourceType resourceType, HeapType heapType, bool unorderedAccess, int size, int elementSize, const wchar_t* debugName )
{
  return std::unique_ptr< Resource >( new D3DResource( *this, resourceType, heapType, unorderedAccess, size, elementSize, debugName ) );
}

std::unique_ptr< RTBottomLevelAccelerator > D3DDevice::CreateRTBottomLevelAccelerator( CommandList& commandList, Resource& vertexBuffer, int vertexCount, int positionElementSize, int vertexStride, Resource& indexBuffer, int indexSize, int indexCount, bool allowUpdate, bool fastBuild )
{
  return std::unique_ptr< RTBottomLevelAccelerator >( new D3DRTBottomLevelAccelerator( *this, *static_cast< D3DCommandList* >( &commandList ), *static_cast< D3DResource* >( &vertexBuffer ), vertexCount, positionElementSize, vertexStride, *static_cast< D3DResource* >( &indexBuffer ), indexSize, indexCount, allowUpdate, fastBuild ) );
}

std::unique_ptr< RTTopLevelAccelerator > D3DDevice::CreateRTTopLevelAccelerator( CommandList& commandList, std::vector< RTInstance > instances, std::vector< SubAccel > areas )
{
  return std::unique_ptr< RTTopLevelAccelerator >( new D3DRTTopLevelAccelerator( *this, *static_cast< D3DCommandList* >( &commandList ), std::move( instances ), std::move( areas ) ) );
}

std::unique_ptr< Resource > D3DDevice::CreateVolumeTexture( CommandList& commandList, int width, int height, int depth, const void* data, int dataSize, PixelFormat format, int slot, std::optional< int > uavSlot, const wchar_t* debugName )
{
  auto resource = CreateTexture( commandList, width, height, depth, data, dataSize, format, false, slot, uavSlot, false, debugName );

  if ( data )
  {
    auto texelSize = CalcTexelSize( format );
    D3D12_SUBRESOURCE_DATA d3dSubresource = { data, width * texelSize, width * height * texelSize };
    D3DDeviceHelper::FillTexture( *static_cast< D3DCommandList* >( &commandList ), *this, *resource, &d3dSubresource, 1, slot );
  }

  return resource;
}

std::unique_ptr< Resource > D3DDevice::Create2DTexture( CommandList& commandList, int width, int height, const void* data, int dataSize, PixelFormat format, bool renderable, int slot, std::optional< int > uavSlot, bool mipLevels, const wchar_t* debugName )
{
  auto resource = CreateTexture( commandList, width, height, 1, data, dataSize, format, renderable, slot, uavSlot, mipLevels, debugName );

  if ( data && !mipLevels )
  {
    auto texelSize = CalcTexelSize( format );
    D3D12_SUBRESOURCE_DATA d3dSubresource = { data, width * texelSize, width * height * texelSize };
    D3DDeviceHelper::FillTexture( *static_cast<D3DCommandList*>( &commandList ), *this, *resource, &d3dSubresource, 1, slot );
  }

  return resource;
}

std::unique_ptr<ComputeShader> D3DDevice::CreateComputeShader( const void* shaderData, int shaderSize, const wchar_t* debugName )
{
  return std::unique_ptr< ComputeShader >( new D3DComputeShader( *this, shaderData, shaderSize, debugName ) );
}

std::unique_ptr<Resource> D3DDevice::Load2DTexture( CommandList& commandList, std::vector< uint8_t >&& textureData, int slot, const wchar_t* debugName, void* customHeap )
{
  CComPtr< ID3D12Resource > d3dResource;
  std::vector< D3D12_SUBRESOURCE_DATA > d3dSubresources;
  bool isCubeMap;
  if FAILED( LoadDDSTextureFromMemory( d3dDevice, textureData.data(), textureData.size(), &d3dResource, d3dSubresources, 0, nullptr, &isCubeMap ) )
    return nullptr;

  if ( debugName )
    d3dResource->SetName( debugName );

  assert( !isCubeMap );
  if ( isCubeMap )
    return nullptr;

  D3D12_RESOURCE_DESC desc = d3dResource->GetDesc();
  assert( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D );
  if( desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D )
    return nullptr;

  CComPtr< ID3D12Resource2 > d3dResource2;
  d3dResource.QueryInterface( &d3dResource2 );
  std::unique_ptr< D3DResource > resource( new D3DResource( d3dResource2, ResourceStateBits::CopyDestination ) );

  if ( customHeap )
  {
    ID3D12DescriptorHeap* d3dHeap = static_cast< ID3D12DescriptorHeap* >( customHeap );

    D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
    desc.Format                  = ConvertForSRV( desc.Format );
    desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    desc.ViewDimension           = D3D12_SRV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipLevels     = -1;

    auto handleSize = d3dDevice->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  
    cpuHandle = d3dHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += slot * handleSize;
    gpuHandle = d3dHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += slot * handleSize;

    d3dDevice->CreateShaderResourceView( d3dResource2, &desc, cpuHandle );

    std::unique_ptr< D3DResourceDescriptor > descriptor( new D3DResourceDescriptor( cpuHandle, gpuHandle ) );
    resource->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( descriptor ) );
  }
  else
  {
    auto& heap       = descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ];
    auto  descriptor = heap->RequestDescriptor( *this, ResourceDescriptorType::ShaderResourceView, slot, *resource, 0 );
    resource->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( descriptor ) );
  }

  D3DDeviceHelper::FillTexture( *static_cast< D3DCommandList* >( &commandList ), *this, *resource, d3dSubresources.data(), int( d3dSubresources.size() ), slot );

  return resource;
}

std::unique_ptr< Resource > D3DDevice::LoadCubeTexture( CommandList& commandList, std::vector< uint8_t >&& textureData, int slot, const wchar_t* debugName )
{
  CComPtr< ID3D12Resource > d3dResource;
  std::vector< D3D12_SUBRESOURCE_DATA > d3dSubresources;
  bool isCubeMap;
  if FAILED( LoadDDSTextureFromMemory( d3dDevice, textureData.data(), textureData.size(), &d3dResource, d3dSubresources, 0, nullptr, &isCubeMap ) )
    return nullptr;

  if ( debugName )
    d3dResource->SetName( debugName );

  assert( isCubeMap );
  if ( !isCubeMap )
    return nullptr;

  CComPtr< ID3D12Resource2 > d3dResource2;
  d3dResource.QueryInterface( &d3dResource2 );
  std::unique_ptr< D3DResource > resource( new D3DResource( d3dResource2, ResourceStateBits::CopyDestination ) );

  auto& heap = descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ];
  auto  descriptor = heap->RequestDescriptor( *this, ResourceDescriptorType::ShaderResourceView, slot, *resource, 0 );
  resource->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( descriptor ) );

  D3D12_RESOURCE_DESC desc = d3dResource->GetDesc();
  assert( desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D );
  if( desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D )
    return nullptr;

  D3DDeviceHelper::FillTexture( *static_cast< D3DCommandList* >( &commandList ), *this, *resource, d3dSubresources.data(), int( d3dSubresources.size() ), slot );

  return resource;
}

DescriptorHeap& D3DDevice::GetShaderResourceHeap()
{
  return *descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ];
}

int D3DDevice::GetUploadSizeForResource( Resource& resource )
{
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
  UINT numRows;
  UINT64 rowSize;
  UINT64 totalBytes;

  auto desc = static_cast< D3DResource* >( &resource )->GetD3DResource()->GetDesc();
  d3dDevice->GetCopyableFootprints( &desc, 0, 1, 0, &layout, &numRows, &rowSize, &totalBytes );

  return int( totalBytes );
}

ID3D12Resource2* D3DDevice::RequestD3DRTScartchBuffer( D3DCommandList& commandList, int size )
{
  if ( d3dRTScratchBufferSize < size )
  {
    if ( d3dRTScartchBuffer )
      commandList.HoldResource( d3dRTScartchBuffer );

    d3dRTScartchBuffer     = AllocateUAVBuffer( d3dDevice, size, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, L"RTScratch" );
    d3dRTScratchBufferSize = int( d3dRTScartchBuffer->GetDesc1().Width );
  }

  return d3dRTScartchBuffer;
}

ID3D12DeviceX* D3DDevice::GetD3DDevice()
{
  return d3dDevice;
}

ID3D12DescriptorHeap* D3DDevice::GetD3DDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE type )
{
  return descriptorHeaps[ int( type ) ]->d3dHeap;
}

ID3D12DescriptorHeap* D3DDevice::GetD3DDearImGuiHeap()
{
  return d3dDearImGuiHeap;
}

ID3D12RootSignature* D3DDevice::GetMipMapGenD3DRootSignature()
{
  return mipmapGenComputeShader->GetD3DRootSignature();
}

ID3D12PipelineState* D3DDevice::GetMipMapGenD3DPipelineState()
{
  return mipmapGenComputeShader->GetD3DPipelineState();
}

ID3D12DescriptorHeap* D3DDevice::GetMipMapGenD3DDescriptorHeap()
{
  return d3dmipmapGenHeap;
}

int D3DDevice::GetMipMapGenDescCounter()
{
  auto c = mipmapGenDescCounter;
  mipmapGenDescCounter = ++mipmapGenDescCounter % mipmapGenHeapSize;
  return c;
}

void D3DDevice::DearImGuiNewFrame()
{
  ImGui_ImplDX12_NewFrame();
}

void* D3DDevice::GetDearImGuiHeap()
{
  return GetD3DDearImGuiHeap();
}

std::unique_ptr< D3DResource > D3DDevice::CreateTexture( CommandList& commandList, int width, int height, int depth, const void* data, int dataSize, PixelFormat format, bool renderable, int slot, std::optional< int > uavSlot, bool mipLevels, const wchar_t* debugName )
{
  bool isVolumeTexture = depth > 1;

  ResourceState initialState;
  if ( IsDepthFormat( format ) )
    initialState.bits = ResourceStateBits::DepthWrite;
  if ( renderable )
    initialState.bits = ResourceStateBits::RenderTarget;
  if ( uavSlot.has_value() )
    initialState.bits = ResourceStateBits::UnorderedAccess;
  
  if ( initialState.bits == 0 )
    initialState.bits = ResourceStateBits::CopyDestination;

  auto flags = D3D12_RESOURCE_FLAG_NONE;
  if ( IsDepthFormat( format ) )
    flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  else if ( renderable )
    flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
  if ( uavSlot.has_value() )
    flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  CComPtr< ID3D12Resource2 > d3dResource;

  D3D12_HEAP_PROPERTIES heapProperties = {};
  heapProperties.Type                 = D3D12_HEAP_TYPE_DEFAULT;
  heapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  heapProperties.CreationNodeMask     = 1;
  heapProperties.VisibleNodeMask      = 1;

  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension          = isVolumeTexture ? D3D12_RESOURCE_DIMENSION_TEXTURE3D : D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Alignment          = 0;
  desc.Width              = width;
  desc.Height             = height;
  desc.DepthOrArraySize   = depth;
  desc.MipLevels          = mipLevels ? 0 : 1;
  desc.Format             = Convert( format );
  desc.SampleDesc.Count   = 1;
  desc.SampleDesc.Quality = 0;
  desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  desc.Flags              = flags;

  D3D12_CLEAR_VALUE optimizedClearValue = {};
  optimizedClearValue.Format = ConvertForDSV( Convert( format ) );
  if ( IsDepthFormat( format ) )
    optimizedClearValue.DepthStencil = { 1.0f, 0 };
  else if ( renderable )
  {
    optimizedClearValue.Color[ 0 ] = 0;
    optimizedClearValue.Color[ 1 ] = 0;
    optimizedClearValue.Color[ 2 ] = 0;
    optimizedClearValue.Color[ 3 ] = 0;
  }

  d3dDevice->CreateCommittedResource( &heapProperties
                                    , D3D12_HEAP_FLAG_NONE
                                    , &desc
                                    , Convert( initialState )
                                    , IsDepthFormat( format ) || renderable ? &optimizedClearValue : nullptr
                                    , IID_PPV_ARGS( &d3dResource ) );

  if ( debugName )
    d3dResource->SetName( debugName );

  std::unique_ptr< D3DResource > resource( new D3DResource( d3dResource, initialState ) );

  auto& heap       = descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ];
  auto  descriptor = heap->RequestDescriptor( *this, ResourceDescriptorType::ShaderResourceView, slot, *resource, 0 );
  resource->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( descriptor ) );

  if ( IsDepthFormat( format ) )
  {
    auto& heap       = descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_DSV ];
    auto  descriptor = heap->RequestDescriptor( *this, ResourceDescriptorType::DepthStencilView, -1, *resource, 0 );
    resource->AttachResourceDescriptor( ResourceDescriptorType::DepthStencilView, std::move( descriptor ) );
  }
  if ( renderable )
  {
    auto& heap       = descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_RTV ];
    auto  descriptor = heap->RequestDescriptor( *this, ResourceDescriptorType::RenderTargetView, -1, *resource, 0 );
    resource->AttachResourceDescriptor( ResourceDescriptorType::RenderTargetView, std::move( descriptor ) );
  }

  if ( uavSlot.value_or( -1 ) >= 0 )
  {
    auto& heap = descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ];
    auto  descriptor = heap->RequestDescriptor( *this, ResourceDescriptorType::UnorderedAccessView, *uavSlot, *resource, 0 );
    resource->AttachResourceDescriptor( ResourceDescriptorType::UnorderedAccessView, std::move( descriptor ) );

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                 = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.Transition.pResource = d3dResource;

    static_cast< D3DCommandList* >( &commandList )->GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );
  }

  return resource;
}
