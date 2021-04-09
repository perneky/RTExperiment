#pragma once

#include "Types.h"

struct CommandQueue;
struct CommandAllocator;
struct CommandList;
struct PipelineState;
struct Resource;
struct RTBottomLevelAccelerator;
struct RTTopLevelAccelerator;
struct DescriptorHeap;
struct ComputeShader;
struct GPUTimeQuery;

struct Device
{
  virtual ~Device() = default;

  virtual std::unique_ptr< CommandQueue >             CreateCommandQueue( CommandQueueType type ) = 0;
  virtual std::unique_ptr< CommandAllocator >         CreateCommandAllocator( CommandQueueType type ) = 0;
  virtual std::unique_ptr< CommandList >              CreateCommandList( CommandAllocator& commandAllocator, CommandQueueType queueType, uint64_t queueFrequency ) = 0;
  virtual std::unique_ptr< PipelineState >            CreatePipelineState( const PipelineDesc& desc ) = 0;
  virtual std::unique_ptr< Resource >                 CreateBuffer( ResourceType resourceType, HeapType heapType, bool unorderedAccess, int size, int elementSize, const wchar_t* debugName ) = 0;
  virtual std::unique_ptr< RTBottomLevelAccelerator > CreateRTBottomLevelAccelerator( CommandList& commandList, Resource& vertexBuffer, int vertexCount, int positionElementSize, int vertexStride, Resource& indexBuffer, int indexSize, int indexCount, bool allowUpdate, bool fastBuild ) = 0;
  virtual std::unique_ptr< RTTopLevelAccelerator >    CreateRTTopLevelAccelerator( CommandList& commandList, std::vector< RTInstance > instances, std::vector< SubAccel > areas ) = 0;
  virtual std::unique_ptr< Resource >                 CreateVolumeTexture( CommandList& commandList, int width, int height, int depth, const void* data, int dataSize, PixelFormat format, int slot, std::optional< int > uavSlot, const wchar_t* debugName ) = 0;
  virtual std::unique_ptr< Resource >                 Create2DTexture( CommandList& commandList, int width, int height, const void* data, int dataSize, PixelFormat format, bool renderable, int slot, std::optional< int > uavSlot, bool mipLevels, const wchar_t* debugName ) = 0;
  virtual std::unique_ptr< ComputeShader >            CreateComputeShader( const void* shaderData, int shaderSize, const wchar_t* debugName ) = 0;
  virtual std::unique_ptr< GPUTimeQuery >             CreateGPUTimeQuery() = 0;

  virtual std::unique_ptr< Resource > Load2DTexture( CommandList& commandList, std::vector< uint8_t >&& textureData, int slot, const wchar_t* debugName, void* customHeap = nullptr ) = 0;
  virtual std::unique_ptr< Resource > LoadCubeTexture( CommandList& commandList, std::vector< uint8_t >&& textureData, int slot, const wchar_t* debugName ) = 0;

  virtual DescriptorHeap& GetShaderResourceHeap() = 0;
  virtual DescriptorHeap& GetSamplerHeap() = 0;

  virtual int GetUploadSizeForResource( Resource& resource ) = 0;

  virtual void SetTextureLODBias( float bias ) = 0;

  virtual void  DearImGuiNewFrame() = 0;
  virtual void* GetDearImGuiHeap() = 0;
};
