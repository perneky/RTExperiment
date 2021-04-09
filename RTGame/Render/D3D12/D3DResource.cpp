#include "D3DResource.h"
#include "D3DCommandList.h"
#include "D3DDevice.h"
#include "D3DDescriptorHeap.h"
#include "D3DResourceDescriptor.h"
#include "Conversion.h"

static int ToIndex( ResourceDescriptorType type )
{
  switch ( type )
  {
  case ResourceDescriptorType::ConstantBufferView:
  case ResourceDescriptorType::ShaderResourceView:
    return 0;

  case ResourceDescriptorType::RenderTargetView:
    return 1;

  case ResourceDescriptorType::DepthStencilView:
    return 2;

  case ResourceDescriptorType::UnorderedAccessView:
    return 3;

  default:
    assert( false );
    return 0;
  }
}

D3DResource::D3DResource( ID3D12Resource2* d3dResource, ResourceState initialState )
  : resourceState( initialState )
  , d3dResource( d3dResource )
{
  auto desc = d3dResource->GetDesc1();

  switch ( desc.Dimension )
  {
  case D3D12_RESOURCE_DIMENSION_BUFFER:    resourceType = ResourceType::ConstantBuffer; break;
  case D3D12_RESOURCE_DIMENSION_TEXTURE1D: resourceType = ResourceType::Texture1D; break;
  case D3D12_RESOURCE_DIMENSION_TEXTURE2D: resourceType = ResourceType::Texture2D; break;
  case D3D12_RESOURCE_DIMENSION_TEXTURE3D: resourceType = ResourceType::Texture3D; break;
  }
  
  isUploadResource = false;
}

D3DResource::D3DResource( D3DDevice& device, ResourceType resourceType, HeapType heapType, bool unorderedAccess, int size, int elementSize, const wchar_t* debugName )
  : resourceType( resourceType )
  , isUploadResource( heapType == HeapType::Upload )
{
  size = ( size + 255 ) & ~255;    // CB size is required to be 256-byte aligned.

  resourceState = heapType == HeapType::Upload ? ResourceStateBits::GenericRead : ( resourceType == ResourceType::IndexBuffer ? ResourceStateBits::IndexBuffer : ResourceStateBits::VertexOrConstantBuffer );
  resourceState = heapType == HeapType::Readback ? ResourceStateBits::CopyDestination : resourceState;

  D3D12_HEAP_PROPERTIES streamHeapProperties = {};
  streamHeapProperties.Type                  = Convert( heapType );
  streamHeapProperties.CPUPageProperty       = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  streamHeapProperties.MemoryPoolPreference  = D3D12_MEMORY_POOL_UNKNOWN;
  streamHeapProperties.CreationNodeMask      = 1;
  streamHeapProperties.VisibleNodeMask       = 1;

  D3D12_RESOURCE_DESC streamResourceDesc = {};
  streamResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
  streamResourceDesc.Alignment          = 0;
  streamResourceDesc.Width              = size;
  streamResourceDesc.Height             = 1;
  streamResourceDesc.DepthOrArraySize   = 1;
  streamResourceDesc.MipLevels          = 1;
  streamResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
  streamResourceDesc.SampleDesc.Count   = 1;
  streamResourceDesc.SampleDesc.Quality = 0;
  streamResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  streamResourceDesc.Flags              = unorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

  auto d3dDevice = static_cast< D3DDevice* >( &device )->GetD3DDevice();
  d3dDevice->CreateCommittedResource( &streamHeapProperties
                                    , D3D12_HEAP_FLAG_NONE
                                    , &streamResourceDesc
                                    , Convert( resourceState )
                                    , nullptr
                                    , IID_PPV_ARGS( &d3dResource ) );

  d3dResource->SetName( debugName );

  if ( resourceType == ResourceType::IndexBuffer )
  {
    assert( elementSize == 1 || elementSize == 2 || elementSize == 4 );
    ibView.BufferLocation = d3dResource->GetGPUVirtualAddress();
    ibView.SizeInBytes    = size;
    ibView.Format         = elementSize == 1 ? DXGI_FORMAT_R8_UINT : ( elementSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT );
  }
  else if ( resourceType == ResourceType::VertexBuffer )
  {
    vbView.BufferLocation = d3dResource->GetGPUVirtualAddress();
    vbView.SizeInBytes    = size;
    vbView.StrideInBytes  = elementSize;
  }
  else if ( resourceType == ResourceType::ConstantBuffer )
  {
  }
  else
    assert( false );
}

D3DResource::~D3DResource()
{
  if ( onDelete )
    onDelete();
}

void D3DResource::AttachResourceDescriptor( ResourceDescriptorType type, std::unique_ptr< ResourceDescriptor > descriptor )
{
  assert( !resourceDescriptors[ ToIndex( type ) ] );
  resourceDescriptors[ ToIndex( type ) ] = std::move( descriptor );
}

ResourceDescriptor* D3DResource::GetResourceDescriptor( ResourceDescriptorType type )
{
  return resourceDescriptors[ ToIndex( type ) ].get();
}

void D3DResource::RemoveResourceDescriptor( ResourceDescriptorType type )
{
  assert( resourceDescriptors[ ToIndex( type ) ] );
  resourceDescriptors[ ToIndex( type ) ].reset();
}

void D3DResource::RemoveAllResourceDescriptors()
{
  for ( auto& desc : resourceDescriptors )
    desc.reset();
}

ResourceState D3DResource::GetCurrentResourceState() const
{
  return resourceState;
}

ResourceType D3DResource::GetResourceType() const
{
  return resourceType;
}

bool D3DResource::IsUploadResource() const
{
  return isUploadResource;
}

int D3DResource::GetBufferSize() const
{
  return int( d3dResource->GetDesc1().Width );
}

int D3DResource::GetTextureWidth() const
{
  return int( d3dResource->GetDesc1().Width );
}

int D3DResource::GetTextureHeight() const
{
  return int( d3dResource->GetDesc1().Height );
}

int D3DResource::GetTextureMipLevels() const
{
  return int( d3dResource->GetDesc1().MipLevels );
}

PixelFormat D3DResource::GetTexturePixelFormat() const
{
  switch ( d3dResource->GetDesc1().Format )
  {
  case DXGI_FORMAT_R8_UNORM:
    return PixelFormat::R8UN;
  }

  assert( false );
  return PixelFormat::Unknown;
}

void* D3DResource::GetImTextureId() const
{
  union
  {
    void*                       imguiHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE d3dHandle;
  } convert;

  assert( resourceDescriptors[ ToIndex( ResourceDescriptorType::ShaderResourceView ) ] );
  convert.d3dHandle = static_cast<const D3DResourceDescriptor*>( resourceDescriptors[ ToIndex( ResourceDescriptorType::ShaderResourceView ) ].get() )->GetD3DGPUHandle();

  return convert.imguiHandle;
}

void* D3DResource::Map()
{
  D3D12_RANGE readRange = { 0, 0 };
  void* bufferData = nullptr;
  d3dResource->Map( 0, &readRange, &bufferData );
  return bufferData;
}

void D3DResource::Unmap()
{
  d3dResource->Unmap( 0, nullptr );
}

ID3D12Resource2* D3DResource::GetD3DResource()
{
  return d3dResource;
}

D3D12_VERTEX_BUFFER_VIEW D3DResource::GetD3DVertexBufferView()
{
  return vbView;
}

D3D12_INDEX_BUFFER_VIEW D3DResource::GetD3DIndexBufferView()
{
  return ibView;
}

D3D12_GPU_VIRTUAL_ADDRESS D3DResource::GetD3DGPUVirtualAddress()
{
  return d3dResource->GetGPUVirtualAddress();
}
