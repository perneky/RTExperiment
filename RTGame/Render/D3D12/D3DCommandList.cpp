#include "D3DCommandList.h"
#include "D3DDevice.h"
#include "D3DCommandAllocator.h"
#include "D3DResource.h"
#include "D3DPipelineState.h"
#include "D3DRTTopLevelAccelerator.h"
#include "D3DDescriptorHeap.h"
#include "D3DResourceDescriptor.h"
#include "D3DComputeShader.h"
#include "Conversion.h"
#include "Common/Color.h"
#include "Conversion.h"
#include "../ShaderStructuresNative.h"
#include "pix3.h"
#include "../ShaderValues.h"
#include "../DearImGui/imgui_impl_dx12.h"

static D3D12_SHADING_RATE Convert( VRSBlock block )
{
  switch ( block )
  {
  case VRSBlock::_1x1:
    return D3D12_SHADING_RATE_1X1;
  case VRSBlock::_1x2:
    return D3D12_SHADING_RATE_1X2;
  case VRSBlock::_2x1:
    return D3D12_SHADING_RATE_2X1;
  case VRSBlock::_2x2:
    return D3D12_SHADING_RATE_2X2;
  default:
    assert( false );
    return D3D12_SHADING_RATE_1X1;
  }
}

D3DCommandList::D3DCommandList( D3DDevice& device, D3DCommandAllocator& commandAllocator, CommandQueueType queueType )
{
  device.GetD3DDevice()->CreateCommandList( 1, Convert( queueType ), commandAllocator.GetD3DCommandAllocator(), nullptr, IID_PPV_ARGS( &d3dGraphicsCommandList ) );
  
  if ( queueType == CommandQueueType::Direct )
  {
    ID3D12DescriptorHeap* allHeaps[] = { device.GetD3DDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV )
                                       , device.GetD3DDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ) };
    d3dGraphicsCommandList->SetDescriptorHeaps( _countof( allHeaps ), allHeaps );
  }
}

D3DCommandList::~D3DCommandList()
{
}

void D3DCommandList::AddUAVBarrier( std::initializer_list< Resource* > resources )
{
  D3D12_RESOURCE_BARRIER barriers[ 8 ] = {};
  assert( resources.size() <= _countof( barriers ) );

  int resIx = 0;
  for ( auto res : resources )
  {
    barriers[ resIx ].Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[ resIx ].Transition.pResource   = static_cast< D3DResource* >( res )->GetD3DResource();
    resIx++;
  }

  d3dGraphicsCommandList->ResourceBarrier( UINT( resources.size() ), barriers );
}

void D3DCommandList::ChangeResourceState( Resource& resource, ResourceState newState )
{
  auto d3dResource = static_cast< D3DResource* >( &resource );
  if ( d3dResource->resourceState.bits == newState.bits )
    return;

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource   = d3dResource->GetD3DResource();
  barrier.Transition.StateBefore = Convert( d3dResource->resourceState );
  barrier.Transition.StateAfter  = Convert( newState );
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  d3dGraphicsCommandList->ResourceBarrier( 1, &barrier );

  d3dResource->resourceState = newState;
}

void D3DCommandList::Clear( Resource& texture, const Color& color )
{
  auto d3dTexture    = static_cast< D3DResource* >( &texture );
  auto d3dDescriptor = static_cast< D3DResourceDescriptor* >( d3dTexture->GetResourceDescriptor( ResourceDescriptorType::RenderTargetView ) );
  float rgba[] = { color.r, color.g, color.b, color.a };
  d3dGraphicsCommandList->ClearRenderTargetView( d3dDescriptor->GetD3DCPUHandle(), rgba, 0, nullptr );
}

void D3DCommandList::Clear( Resource& texture, float depth )
{
  auto d3dTexture    = static_cast< D3DResource* >( &texture );
  auto d3dDescriptor = static_cast< D3DResourceDescriptor* >( d3dTexture->GetResourceDescriptor( ResourceDescriptorType::DepthStencilView ) );
  d3dGraphicsCommandList->ClearDepthStencilView( d3dDescriptor->GetD3DCPUHandle(), D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr );
}

void D3DCommandList::SetRenderTarget( Resource& colorBuffer, Resource* depthBuffer )
{
  auto d3dColorTexture    = static_cast< D3DResource* >( &colorBuffer );
  auto d3dColorDescriptor = static_cast< D3DResourceDescriptor* >( d3dColorTexture->GetResourceDescriptor( ResourceDescriptorType::RenderTargetView ) );
  
  D3D12_CPU_DESCRIPTOR_HANDLE rtDescriptors[] = { d3dColorDescriptor->GetD3DCPUHandle() };
  
  if ( depthBuffer )
  {
    auto d3dDepthTexture    = static_cast< D3DResource* >( depthBuffer );
    auto d3dDepthDescriptor = static_cast< D3DResourceDescriptor* >( d3dDepthTexture->GetResourceDescriptor( ResourceDescriptorType::DepthStencilView ) );

    D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor = d3dDepthDescriptor->GetD3DCPUHandle();
    d3dGraphicsCommandList->OMSetRenderTargets( _countof( rtDescriptors ), rtDescriptors, false, &depthDescriptor );
  }
  else
  {
    d3dGraphicsCommandList->OMSetRenderTargets( _countof( rtDescriptors ), rtDescriptors, false, nullptr );
  }
}

void D3DCommandList::SetRenderTarget( const std::vector< Resource* >& colorBuffers, Resource* depthBuffer )
{
  std::vector< D3D12_CPU_DESCRIPTOR_HANDLE > rtDescriptors;
  for ( auto colorBuffer : colorBuffers )
  {
    auto d3dColorTexture    = static_cast< D3DResource* >( colorBuffer );
    auto d3dColorDescriptor = static_cast< D3DResourceDescriptor* >( d3dColorTexture->GetResourceDescriptor( ResourceDescriptorType::RenderTargetView ) );
    rtDescriptors.emplace_back( d3dColorDescriptor->GetD3DCPUHandle() );
  }
  
  if ( depthBuffer )
  {
    auto d3dDepthTexture    = static_cast< D3DResource* >( depthBuffer );
    auto d3dDepthDescriptor = static_cast< D3DResourceDescriptor* >( d3dDepthTexture->GetResourceDescriptor( ResourceDescriptorType::DepthStencilView ) );

    D3D12_CPU_DESCRIPTOR_HANDLE depthDescriptor = d3dDepthDescriptor->GetD3DCPUHandle();
    d3dGraphicsCommandList->OMSetRenderTargets( UINT( rtDescriptors.size() ), rtDescriptors.data(), false, &depthDescriptor );
  }
  else
  {
    d3dGraphicsCommandList->OMSetRenderTargets( UINT( rtDescriptors.size() ), rtDescriptors.data(), false, nullptr );
  }
}

void D3DCommandList::SetViewport( int left, int top, int width, int height )
{
  D3D12_VIEWPORT d3dViewport = { float( left ), float( top ), float( width ), float( height ), 0, 1 };
  d3dGraphicsCommandList->RSSetViewports( 1, &d3dViewport );
}

void D3DCommandList::SetScissor( int left, int top, int width, int height )
{
  D3D12_RECT d3dRect = { left, top, left + width, top + height };
  d3dGraphicsCommandList->RSSetScissorRects( 1, &d3dRect );
}

void D3DCommandList::SetPipelineState( PipelineState& pipelineState )
{
  d3dGraphicsCommandList->SetPipelineState( static_cast< D3DPipelineState* >( &pipelineState )->GetD3DPipelineState() );
  d3dGraphicsCommandList->SetGraphicsRootSignature( static_cast< D3DPipelineState* >( &pipelineState )->GetD3DRootSignature() );
}

void D3DCommandList::SetVertexBuffer( Resource& bufferResource )
{
  auto d3dResource   = static_cast< D3DResource* >( &bufferResource );
  auto d3dBufferView = d3dResource->GetD3DVertexBufferView();
  d3dGraphicsCommandList->IASetVertexBuffers( 0, 1, &d3dBufferView );
}

void D3DCommandList::SetIndexBuffer( Resource& bufferResource )
{
  auto d3dResource   = static_cast< D3DResource* >( &bufferResource );
  auto d3dBufferView = d3dResource->GetD3DIndexBufferView();
  d3dGraphicsCommandList->IASetIndexBuffer( &d3dBufferView );
}

void D3DCommandList::SetConstantBuffer( int index, Resource& bufferResource )
{
  auto d3dResource = static_cast< D3DResource* >( &bufferResource );
  d3dGraphicsCommandList->SetGraphicsRootConstantBufferView( index, d3dResource->GetD3DResource()->GetGPUVirtualAddress() );
}

void D3DCommandList::SetShaderResourceView( int index, Resource& bufferResource )
{
  auto d3dResource = static_cast< D3DResource* >( &bufferResource );
  d3dGraphicsCommandList->SetGraphicsRootShaderResourceView( index, d3dResource->GetD3DResource()->GetGPUVirtualAddress() );
}

void D3DCommandList::SetUnorderedAccessView( int index, Resource& bufferResource )
{
  auto d3dResource   = static_cast< D3DResource* >( &bufferResource );
  auto d3dDescriptor = static_cast< D3DResourceDescriptor* >( d3dResource->GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );
  d3dGraphicsCommandList->SetGraphicsRootDescriptorTable( index, d3dDescriptor->GetD3DGPUHandle() );
}

void D3DCommandList::SetTextureHeap( int index, DescriptorHeap& heap, int offset )
{
  auto d3dHeap   = static_cast< D3DDescriptorHeap* >( &heap );
  auto gpuHandle = d3dHeap->GetD3DHeap()->GetGPUDescriptorHandleForHeapStart();

  gpuHandle.ptr += offset * d3dHeap->GetDescriptorSize();
  d3dGraphicsCommandList->SetGraphicsRootDescriptorTable( index, gpuHandle );
}

void D3DCommandList::SetConstantValues( int index, const void* values, int numValues, int offset )
{
  d3dGraphicsCommandList->SetGraphicsRoot32BitConstants( index, numValues, values, offset );
}

void D3DCommandList::SetRayTracingScene( int index, RTTopLevelAccelerator& accelerator )
{
  auto d3dAccelerator = static_cast< D3DRTTopLevelAccelerator* >( &accelerator );
  d3dGraphicsCommandList->SetGraphicsRootShaderResourceView( index, d3dAccelerator->GetD3DUAVBuffer()->GetGPUVirtualAddress() );
}

void D3DCommandList::SetPrimitiveType( PrimitiveType primitiveType )
{
  d3dGraphicsCommandList->IASetPrimitiveTopology( Convert( primitiveType ) );
}

void D3DCommandList::SetComputeShader( ComputeShader& shader )
{
  auto d3dComputeShader = static_cast< D3DComputeShader* >( &shader );
  d3dGraphicsCommandList->SetPipelineState( d3dComputeShader->GetD3DPipelineState() );
  d3dGraphicsCommandList->SetComputeRootSignature( d3dComputeShader->GetD3DRootSignature() );
}

void D3DCommandList::SetComputeConstantValues( int index, const void* values, int numValues, int offset )
{
  d3dGraphicsCommandList->SetComputeRoot32BitConstants( index, numValues, values, offset );
}

void D3DCommandList::SetComputeConstantBuffer( int index, Resource& bufferResource )
{
  auto d3dResource = static_cast< D3DResource* >( &bufferResource );
  d3dGraphicsCommandList->SetComputeRootConstantBufferView( index, d3dResource->GetD3DResource()->GetGPUVirtualAddress() );
}

void D3DCommandList::SetComputeResource( int index, ResourceDescriptor& descriptor )
{
  auto d3dDescriptor = static_cast< D3DResourceDescriptor* >( &descriptor );
  d3dGraphicsCommandList->SetComputeRootDescriptorTable( index, d3dDescriptor->GetD3DGPUHandle() );
}

void D3DCommandList::SetComputeRayTracingScene( int index, RTTopLevelAccelerator& accelerator )
{
  auto d3dAccelerator = static_cast< D3DRTTopLevelAccelerator* >( &accelerator );
  d3dGraphicsCommandList->SetComputeRootShaderResourceView( index, d3dAccelerator->GetD3DUAVBuffer()->GetGPUVirtualAddress() );
}

void D3DCommandList::SetComputeTextureHeap( int index, DescriptorHeap& heap, int offset )
{
  auto d3dHeap = static_cast< D3DDescriptorHeap* >( &heap );
  auto gpuHandle = d3dHeap->GetD3DHeap()->GetGPUDescriptorHandleForHeapStart();

  gpuHandle.ptr += offset * d3dHeap->GetDescriptorSize();
  d3dGraphicsCommandList->SetComputeRootDescriptorTable( index, gpuHandle );
}

void D3DCommandList::SetVariableRateShading( VRSBlock block )
{
  d3dGraphicsCommandList->RSSetShadingRate( Convert( block ), nullptr );
}

void D3DCommandList::Dispatch( int groupsX, int groupsY, int groupsZ )
{
  d3dGraphicsCommandList->Dispatch( groupsX, groupsY, groupsZ );
}

void D3DCommandList::GenerateMipmaps( Resource& resource )
{
  auto d3dResource = static_cast< D3DResource* >( &resource )->GetD3DResource();
  auto desc = d3dResource->GetDesc1();
  if ( desc.MipLevels == 1 )
    return;

  CComPtr< ID3D12Device8 > d3dDevice;
  d3dGraphicsCommandList->GetDevice( IID_PPV_ARGS( &d3dDevice ) );

  auto device = GetContainerObject< D3DDevice >( d3dDevice );
  auto mipmapGenSig  = device->GetMipMapGenD3DRootSignature();
  auto mipmapGenPSO  = device->GetMipMapGenD3DPipelineState();
  auto mipmapGenHeap = device->GetMipMapGenD3DDescriptorHeap();

  d3dGraphicsCommandList->SetPipelineState( mipmapGenPSO );
  d3dGraphicsCommandList->SetComputeRootSignature( mipmapGenSig );
  d3dGraphicsCommandList->SetDescriptorHeaps( 1, &mipmapGenHeap );

  auto descriptorSize = d3dDevice->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV );
  D3D12_CPU_DESCRIPTOR_HANDLE currentCPUHandle = mipmapGenHeap->GetCPUDescriptorHandleForHeapStart();
  D3D12_GPU_DESCRIPTOR_HANDLE currentGPUHandle = mipmapGenHeap->GetGPUDescriptorHandleForHeapStart();

  auto oldState = resource.GetCurrentResourceState();
  ChangeResourceState( resource, ResourceStateBits::UnorderedAccess );

  for ( int srcMip = 0; srcMip < desc.MipLevels - 1; ++srcMip )
  {
    auto dstWidth  = std::max( int( desc.Width  ) >> ( srcMip + 1 ), 1 );
    auto dstHeight = std::max( int( desc.Height ) >> ( srcMip + 1 ), 1 );

    auto srcSlot = device->GetMipMapGenDescCounter();
    auto dstSlot = device->GetMipMapGenDescCounter();

    auto srcCPULoc = currentCPUHandle;
    auto dstCPULoc = currentCPUHandle;
    auto srcGPULoc = currentGPUHandle;
    auto dstGPULoc = currentGPUHandle;
    
    srcCPULoc.ptr += descriptorSize * srcSlot;
    dstCPULoc.ptr += descriptorSize * dstSlot;
    srcGPULoc.ptr += descriptorSize * srcSlot;
    dstGPULoc.ptr += descriptorSize * dstSlot;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = d3dResource;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barrier.Transition.Subresource = srcMip;
    d3dGraphicsCommandList->ResourceBarrier( 1, &barrier );

    D3D12_SHADER_RESOURCE_VIEW_DESC srcTextureSRVDesc = {};
    srcTextureSRVDesc.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srcTextureSRVDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
    srcTextureSRVDesc.Format                    = desc.Format;
    srcTextureSRVDesc.Texture2D.MipLevels       = 1;
    srcTextureSRVDesc.Texture2D.MostDetailedMip = srcMip;
    d3dDevice->CreateShaderResourceView( d3dResource, &srcTextureSRVDesc, srcCPULoc );

    D3D12_UNORDERED_ACCESS_VIEW_DESC destTextureUAVDesc = {};
    destTextureUAVDesc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE2D;
    destTextureUAVDesc.Format             = desc.Format;
    destTextureUAVDesc.Texture2D.MipSlice = srcMip + 1;
    d3dDevice->CreateUnorderedAccessView( d3dResource, nullptr, &destTextureUAVDesc, dstCPULoc );

    d3dGraphicsCommandList->SetComputeRootDescriptorTable( 0, srcGPULoc );
    d3dGraphicsCommandList->SetComputeRootDescriptorTable( 1, dstGPULoc );

    d3dGraphicsCommandList->Dispatch( ( dstWidth  + DownsamplingKernelWidth  - 1 ) / DownsamplingKernelWidth
                                    , ( dstHeight + DownsamplingKernelHeight - 1 ) / DownsamplingKernelHeight
                                    , 1 );

    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type          = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = d3dResource;
    d3dGraphicsCommandList->ResourceBarrier( 1, &uavBarrier );
  }

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource   = d3dResource;
  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  barrier.Transition.Subresource = desc.MipLevels - 1;
  d3dGraphicsCommandList->ResourceBarrier( 1, &barrier );

  barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
  barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  d3dGraphicsCommandList->ResourceBarrier( 1, &barrier );

  ID3D12DescriptorHeap* allHeaps[] = { device->GetD3DDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV )
                                     , device->GetD3DDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ) };
  d3dGraphicsCommandList->SetDescriptorHeaps( _countof( allHeaps ), allHeaps );
}

void D3DCommandList::DearImGuiRender( Device& device )
{
  auto d3dHeap = static_cast< D3DDevice* >( &device )->GetD3DDearImGuiHeap();
  d3dGraphicsCommandList->SetDescriptorHeaps( 1, &d3dHeap );
  ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), d3dGraphicsCommandList );
}

void D3DCommandList::Draw( int vertexCount, int instanceCount, int startVertex, int startInstance )
{
  d3dGraphicsCommandList->DrawInstanced( vertexCount, instanceCount, startVertex, startInstance );
}

void D3DCommandList::DrawIndexed( int indexCount, int instanceCount, int startIndex, int baseVertex, int startInstance )
{
  d3dGraphicsCommandList->DrawIndexedInstanced( indexCount, instanceCount, startIndex, baseVertex, startInstance );
}

void D3DCommandList::UploadTextureResource( std::unique_ptr< Resource > source, Resource& destination, const void* data, int stride, int rows )
{
  auto oldState = destination.GetCurrentResourceState();
  ChangeResourceState( destination, ResourceStateBits::CopyDestination );

  auto d3dSource      = static_cast< D3DResource* >( source.get() )->GetD3DResource();
  auto d3dDestination = static_cast< D3DResource* >( &destination )->GetD3DResource();

  D3D12_SUBRESOURCE_DATA subresource;
  subresource.pData      = data;
  subresource.RowPitch   = stride;
  subresource.SlicePitch = stride * rows;

  UpdateSubresources( d3dGraphicsCommandList, d3dDestination, d3dSource, 0, 0, 1, &subresource );

  ChangeResourceState( destination, oldState );

  heldResources.emplace_back( std::move( source ) );
}

void D3DCommandList::UploadBufferResource( std::unique_ptr< Resource > source, Resource& destination, const void* data, int dataSize )
{
  auto oldState = destination.GetCurrentResourceState();
  ChangeResourceState( destination, ResourceStateBits::CopyDestination );

  auto d3dSource      = static_cast< D3DResource* >( source.get() )->GetD3DResource();
  auto d3dDestination = static_cast< D3DResource* >( &destination )->GetD3DResource();

  D3D12_SUBRESOURCE_DATA subresource;
  subresource.pData      = data;
  subresource.RowPitch   = dataSize;
  subresource.SlicePitch = dataSize;

  auto uploadSize = source->GetBufferSize();
  std::vector< uint8_t > paddedUploadBuffer;
  if ( dataSize < uploadSize )
  {
    paddedUploadBuffer.resize( uploadSize );
    memcpy_s( paddedUploadBuffer.data(), paddedUploadBuffer.size(), data, dataSize );
    subresource.pData      = paddedUploadBuffer.data();
    subresource.RowPitch   = paddedUploadBuffer.size();
    subresource.SlicePitch = paddedUploadBuffer.size();
  }

  UpdateSubresources( d3dGraphicsCommandList, d3dDestination, d3dSource, 0, 0, 1, &subresource );

  ChangeResourceState( destination, oldState );

  heldResources.emplace_back( std::move( source ) );
}

void D3DCommandList::CopyResource( Resource& source, Resource& destination )
{
  auto oldDstState = destination.GetCurrentResourceState();
  ChangeResourceState( destination, ResourceStateBits::CopyDestination );

  auto oldSrcState = source.GetCurrentResourceState();
  ChangeResourceState( source, ResourceStateBits::CopySource );

  d3dGraphicsCommandList->CopyResource( static_cast< D3DResource* >( &destination )->GetD3DResource(), static_cast< D3DResource* >( &source )->GetD3DResource() );

  ChangeResourceState( destination, oldDstState );
  ChangeResourceState( source, oldSrcState );
}

void D3DCommandList::HoldResource( std::unique_ptr< Resource > resource )
{
  if ( resource )
    heldResources.emplace_back( std::move( resource ) );
}

std::vector< std::unique_ptr< Resource > > D3DCommandList::TakeHeldResources()
{
  return std::move( heldResources );
}

void D3DCommandList::BeginEvent( int eventId, const wchar_t* format, ... )
{
#if ENABLE_GPU_DEBUG
  wchar_t msg[ 512 ];

  va_list args;
  va_start( args, format );
  vswprintf_s( msg, format, args );
  va_end( args );

  PIXBeginEvent( (ID3D12GraphicsCommandList6*)d3dGraphicsCommandList, PIX_COLOR_INDEX( BYTE( eventId ) ), msg );
#endif // ENABLE_GPU_DEBUG
}

void D3DCommandList::EndEvent()
{
#if ENABLE_GPU_DEBUG
  PIXEndEvent( (ID3D12GraphicsCommandList6*)d3dGraphicsCommandList );
#endif // ENABLE_GPU_DEBUG
}

void D3DCommandList::HoldResource( ID3D12Resource2* d3dResource )
{
  HoldResource( std::unique_ptr< Resource >( new D3DResource( d3dResource, ResourceStateBits::Common ) ) );
}

ID3D12GraphicsCommandList6* D3DCommandList::GetD3DGraphicsCommandList()
{
  return d3dGraphicsCommandList;
}
