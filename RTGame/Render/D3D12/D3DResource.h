#pragma once

#include "../Resource.h"

class D3DDescriptorHeap;

class D3DResource : public Resource
{
  friend class D3DCommandList;
  friend class D3DSwapchain;
  friend class D3DDevice;
  friend class D3DRTBottomLevelAccelerator;
  friend class D3DRTTopLevelAccelerator;
  friend struct D3DDeviceHelper;

public:
  ~D3DResource();

  void                AttachResourceDescriptor( ResourceDescriptorType type, std::unique_ptr< ResourceDescriptor > descriptor ) override;
  ResourceDescriptor* GetResourceDescriptor( ResourceDescriptorType type ) override;
  void                RemoveResourceDescriptor( ResourceDescriptorType type ) override;
  void                RemoveAllResourceDescriptors() override;

  ResourceState GetCurrentResourceState() const override;

  ResourceType GetResourceType() const override;
  bool IsUploadResource() const override;

  int GetBufferSize() const override;

  int GetTextureWidth() const override;
  int GetTextureHeight() const override;
  int GetTextureMipLevels() const override;
  PixelFormat GetTexturePixelFormat() const override;

  void* GetImTextureId() const override;

  void* Map() override;
  void  Unmap() override;

  ID3D12Resource2*            GetD3DResource();
  D3D12_VERTEX_BUFFER_VIEW    GetD3DVertexBufferView();
  D3D12_INDEX_BUFFER_VIEW     GetD3DIndexBufferView();
  D3D12_GPU_VIRTUAL_ADDRESS   GetD3DGPUVirtualAddress();

protected:
  D3DResource( ID3D12Resource2* d3dResource, ResourceState initialState );
  D3DResource( D3DDevice& device, ResourceType resourceType, HeapType heapType, bool unorderedAccess, int size, int elementSize, const wchar_t* debugName );

  ResourceType resourceType;
  bool         isUploadResource;

  ResourceState                         resourceState;
  CComPtr< ID3D12Resource2 >            d3dResource;
  int                                   descriptorIndex;
  
  std::unique_ptr< ResourceDescriptor > resourceDescriptors[ 4 ];

  std::function< void() > onDelete;

  union
  {
    D3D12_VERTEX_BUFFER_VIEW vbView;
    D3D12_INDEX_BUFFER_VIEW  ibView;
  };
};