#pragma once

#include "../CommandList.h"
#include "../Types.h"

class D3DCommandAllocator;
struct AllocatedResource;

namespace D3D12MA { class Allocation; }

class D3DCommandList : public CommandList
{
  friend class D3DDevice;

public:
  ~D3DCommandList();

  void BindHeaps( Device& device ) override;

  void ChangeResourceState( Resource& resource, ResourceState newState ) override;

  void Clear( Resource& texture, const Color& color ) override;
  void Clear( Resource& texture, float depth ) override;
  
  void SetRenderTarget( Resource& colorBuffer, Resource* depthBuffer ) override;
  void SetRenderTarget( const std::vector< Resource* >& colorBuffers, Resource* depthBuffer ) override;
  void SetViewport( int left, int top, int width, int height ) override;
  void SetScissor( int left, int top, int width, int height ) override;
  void SetPipelineState( PipelineState& pipelineState ) override;
  void SetVertexBuffer( Resource& bufferResource ) override;
  void SetIndexBuffer( Resource& bufferResource ) override;
  void SetConstantBuffer( int index, Resource& bufferResource ) override;
  void SetShaderResourceView( int index, Resource& bufferResource ) override;
  void SetUnorderedAccessView( int index, Resource& bufferResource ) override;
  void SetDescriptorHeap( int index, DescriptorHeap& heap, int offset ) override;
  void SetConstantValues( int index, const void* values, int numValues, int offset ) override;
  void SetRayTracingScene( int index, RTTopLevelAccelerator& accelerator ) override;
  void SetPrimitiveType( PrimitiveType primitiveType ) override;

  void Draw( int vertexCount, int instanceCount = 1, int startVertex = 0, int startInstance = 0 ) override;
  void DrawIndexed( int indexCount, int instanceCount, int startIndex, int baseVertex, int startInstance ) override;

  void SetComputeShader( ComputeShader& shader ) override;
  void SetComputeConstantValues( int index, const void* values, int numValues, int offset ) override;
  void SetComputeConstantBuffer( int index, Resource& bufferResource ) override;
  void SetComputeResource( int index, ResourceDescriptor& descriptor ) override;
  void SetComputeRayTracingScene( int index, RTTopLevelAccelerator& accelerator ) override;
  void SetComputeTextureHeap( int index, DescriptorHeap& heap, int offset ) override;

  void SetVariableRateShading( VRSBlock block ) override;

  void Dispatch( int groupsX, int groupsY, int groupsZ ) override;

  void AddUAVBarrier( std::initializer_list< Resource* > resources ) override;

  void GenerateMipmaps( Resource& resource ) override;

  void DearImGuiRender( Device& device ) override;

  void UploadTextureResource( std::unique_ptr< Resource > source, Resource& destination, const void* data, int stride, int rows ) override;
  void UploadBufferResource( std::unique_ptr< Resource > source, Resource& destination, const void* data, int dataSize ) override;

  void CopyResource( Resource& source, Resource& destination ) override;

  void HoldResource( std::unique_ptr< Resource > resource ) override;

  std::vector< std::unique_ptr< Resource > > TakeHeldResources() override;

  void BeginEvent( int eventId, const wchar_t* format, ... ) override;
  void EndEvent() override;

  void RegisterEndFrameCallback( EndFrameCallback&& callback ) override;
  std::vector< EndFrameCallback > TakeEndFrameCallbacks() override;

  void HoldResource( D3D12MA::Allocation* allocation );
  void HoldResource( AllocatedResource&& allocation );

  ID3D12GraphicsCommandList6* GetD3DGraphicsCommandList();

  uint64_t GetFrequency() const;

private:
  D3DCommandList( D3DDevice& device, D3DCommandAllocator& commandAllocator, CommandQueueType queueType, uint64_t queueFrequency );

  CComPtr< ID3D12GraphicsCommandList6 > d3dGraphicsCommandList;

  std::vector< std::unique_ptr< Resource > > heldResources;
  std::vector< EndFrameCallback > endFrameCallbacks;

  uint64_t frequency = 1;
};