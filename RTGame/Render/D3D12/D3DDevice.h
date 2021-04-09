#pragma once

#include "../Device.h"

class D3DDescriptorHeap;
class D3DCommandList;
class D3DComputeShader;
class D3DResource;

class D3DDevice : public Device
{
  friend class D3DAdapter;
  friend struct D3DDeviceHelper;

public:
  ~D3DDevice();

  std::unique_ptr< CommandQueue >             CreateCommandQueue( CommandQueueType type ) override;
  std::unique_ptr< CommandAllocator >         CreateCommandAllocator( CommandQueueType type ) override;
  std::unique_ptr< CommandList >              CreateCommandList( CommandAllocator& commandAllocator, CommandQueueType queueType, uint64_t queueFrequency ) override;
  std::unique_ptr< PipelineState >            CreatePipelineState( const PipelineDesc& desc ) override;
  std::unique_ptr< Resource >                 CreateBuffer( ResourceType resourceType, HeapType heapType, bool unorderedAccess, int size, int elementSize, const wchar_t* debugName ) override;
  std::unique_ptr< RTBottomLevelAccelerator > CreateRTBottomLevelAccelerator( CommandList& commandList, Resource& vertexBuffer, int vertexCount, int positionElementSize, int vertexStride, Resource& indexBuffer, int indexSize, int indexCount, bool allowUpdate, bool fastBuild ) override;
  std::unique_ptr< RTTopLevelAccelerator >    CreateRTTopLevelAccelerator( CommandList& commandList, std::vector< RTInstance > instances, std::vector< SubAccel > areas ) override;
  std::unique_ptr< Resource >                 CreateVolumeTexture( CommandList& commandList, int width, int height, int depth, const void* data, int dataSize, PixelFormat format, int slot, std::optional< int > uavSlot, const wchar_t* debugName ) override;
  std::unique_ptr< Resource >                 Create2DTexture( CommandList& commandList, int width, int height, const void* data, int dataSize, PixelFormat format, bool renderable, int slot, std::optional< int > uavSlot, bool mipLevels, const wchar_t* debugName ) override;
  std::unique_ptr< ComputeShader >            CreateComputeShader( const void* shaderData, int shaderSize, const wchar_t* debugName ) override;
  std::unique_ptr< GPUTimeQuery >             CreateGPUTimeQuery() override;

  std::unique_ptr< Resource > Load2DTexture( CommandList& commandList, std::vector< uint8_t >&& textureData, int slot, const wchar_t* debugName, void* customHeap = nullptr ) override;
  std::unique_ptr< Resource > LoadCubeTexture( CommandList& commandList, std::vector< uint8_t >&& textureData, int slot, const wchar_t* debugName ) override;

  DescriptorHeap& GetShaderResourceHeap() override;
  DescriptorHeap& GetSamplerHeap() override;

  int GetUploadSizeForResource( Resource& resource ) override;

  void SetTextureLODBias( float bias ) override;

  void  DearImGuiNewFrame() override;
  void* GetDearImGuiHeap() override;

  ID3D12Resource2* RequestD3DRTScartchBuffer( D3DCommandList& commandList, int size );

  ID3D12DeviceX*        GetD3DDevice();
  ID3D12DescriptorHeap* GetD3DDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE type );

  ID3D12DescriptorHeap* GetD3DDearImGuiHeap();

  ID3D12RootSignature*  GetMipMapGenD3DRootSignature();
  ID3D12PipelineState*  GetMipMapGenD3DPipelineState();
  ID3D12DescriptorHeap* GetMipMapGenD3DDescriptorHeap();
  int                   GetMipMapGenDescCounter();

private:
  D3DDevice( D3DAdapter& adapter );

  void UpdateSamplers();

  std::unique_ptr< D3DResource > CreateTexture( CommandList& commandList, int width, int height, int depth, const void* data, int dataSize, PixelFormat format, bool renderable, int slot, std::optional< int > uavSlot, bool mipLevels, const wchar_t* debugName );

  CComPtr< ID3D12DeviceX > d3dDevice;

  std::unique_ptr< D3DDescriptorHeap > descriptorHeaps[ D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ];

  CComPtr< ID3D12Resource2 > d3dRTScartchBuffer;
  int                        d3dRTScratchBufferSize = 0;

  CComPtr< ID3D12DescriptorHeap > d3dDearImGuiHeap;

  std::unique_ptr< D3DComputeShader > mipmapGenComputeShader;
  CComPtr< ID3D12DescriptorHeap >     d3dmipmapGenHeap;
  int                                 mipmapGenDescCounter;

  float textureLODBias = 0;
};
