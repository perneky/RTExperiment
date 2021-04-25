#include "D3DDescriptorHeap.h"
#include "D3DDevice.h"
#include "D3DResourceDescriptor.h"
#include "D3DResource.h"

D3DDescriptorHeap::D3DDescriptorHeap( D3DDevice& device, int freeAutoDescriptorStart, int descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType, const wchar_t* debugName )
  : freeAutoDescriptorStart( freeAutoDescriptorStart )
{
  for ( int index = 0; index < descriptorCount; ++index )
    freeDescriptors.emplace( index );

  D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
  heapDesc.NumDescriptors = descriptorCount;
  heapDesc.Flags          = heapType < D3D12_DESCRIPTOR_HEAP_TYPE_RTV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  heapDesc.Type           = heapType;
  device.GetD3DDevice()->CreateDescriptorHeap( &heapDesc, IID_PPV_ARGS( &d3dHeap ) );

  d3dHeap->SetName( debugName );

  handleSize = device.GetD3DDevice()->GetDescriptorHandleIncrementSize( heapType );
}

D3DDescriptorHeap::~D3DDescriptorHeap()
{
}

std::unique_ptr< ResourceDescriptor > D3DDescriptorHeap::RequestDescriptor( Device& device, ResourceDescriptorType type, int slot, Resource& resource, int bufferElementSize, int mipLevel )
{
  std::lock_guard< std::mutex > autoLock( descriptorLock );
  
  assert( !freeDescriptors.empty() );

  if ( slot >= 0 )
  {
    bool isFree = freeDescriptors.count( slot ) == 1;
    assert( isFree );
    if ( !isFree )
      return nullptr;

    freeDescriptors.erase( slot );
  }
  else
  {
    slot = *freeDescriptors.lower_bound( freeAutoDescriptorStart );
    freeDescriptors.erase( slot );
  }

  D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
  D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
  
  cpuHandle = d3dHeap->GetCPUDescriptorHandleForHeapStart();
  cpuHandle.ptr += slot * handleSize;
  gpuHandle = d3dHeap->GetGPUDescriptorHandleForHeapStart();
  gpuHandle.ptr += slot * handleSize;

  return std::unique_ptr< ResourceDescriptor >( new D3DResourceDescriptor( *static_cast< D3DDevice* >( &device ), *this, type, slot, *static_cast< D3DResource* >( &resource ), mipLevel, bufferElementSize, cpuHandle, gpuHandle ) );
}

int D3DDescriptorHeap::GetDescriptorSize() const
{
  return int( handleSize );
}

void D3DDescriptorHeap::FreeDescriptor( int index )
{
  std::lock_guard< std::mutex > autoLock( descriptorLock );

  freeDescriptors.emplace( index );
}

ID3D12DescriptorHeap* D3DDescriptorHeap::GetD3DHeap()
{
  return d3dHeap;
}
