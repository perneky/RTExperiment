#pragma once

#include "../DescriptorHeap.h"

class D3DDescriptorHeap : public DescriptorHeap
{
  friend class D3DDevice;

public:
  ~D3DDescriptorHeap();

  std::unique_ptr< ResourceDescriptor > RequestDescriptor( Device& device, ResourceDescriptorType type, int slot, Resource& resource, int bufferElementSize, int mipLevel = 0 ) override;

  int GetDescriptorSize() const override;

  void FreeDescriptor( int index );

  ID3D12DescriptorHeap* GetD3DHeap();

private:
  D3DDescriptorHeap( D3DDevice& device, int freeAutoDescriptorStart, int descriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType, const wchar_t* debugName );

  int freeAutoDescriptorStart;

  CComPtr< ID3D12DescriptorHeap > d3dHeap;

  std::mutex      descriptorLock;
  std::set< int > freeDescriptors;

  size_t handleSize = 0;
};