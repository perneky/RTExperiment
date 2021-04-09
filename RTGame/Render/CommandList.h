#pragma once

#include "Types.h"

struct Resource;
struct PipelineState;
struct Color;
struct RTTopLevelAccelerator;
struct DescriptorHeap;
struct ComputeShader;
struct ResourceDescriptor;
struct Device;

enum class VRSBlock : int;

struct CommandList
{
  virtual ~CommandList() = default;

  virtual void BindHeaps( Device& device ) = 0;

  virtual void ChangeResourceState( Resource& resource, ResourceState newState ) = 0;

  virtual void Clear( Resource& texture, const Color& color ) = 0;
  virtual void Clear( Resource& texture, float depth ) = 0;

  virtual void SetRenderTarget( Resource& colorBuffer, Resource* depthBuffer ) = 0;
  virtual void SetRenderTarget( const std::vector< Resource* >& colorBuffers, Resource* depthBuffer ) = 0;
  virtual void SetViewport( int left, int top, int width, int height ) = 0;
  virtual void SetScissor( int left, int top, int width, int height ) = 0;
  virtual void SetPipelineState( PipelineState& pipelineState ) = 0;
  virtual void SetVertexBuffer( Resource& bufferResource ) = 0;
  virtual void SetIndexBuffer( Resource& bufferResource ) = 0;
  virtual void SetConstantBuffer( int index, Resource& bufferResource ) = 0;
  virtual void SetShaderResourceView( int index, Resource& bufferResource ) = 0;
  virtual void SetUnorderedAccessView( int index, Resource& bufferResource ) = 0;
  virtual void SetDescriptorHeap( int index, DescriptorHeap& heap, int offset ) = 0;
  virtual void SetConstantValues( int index, const void* values, int numValues, int offset = 0 ) = 0;
  virtual void SetRayTracingScene( int index, RTTopLevelAccelerator& accelerator ) = 0;
  virtual void SetPrimitiveType( PrimitiveType primitiveType ) = 0;

  virtual void Draw( int vertexCount, int instanceCount = 1, int startVertex = 0, int startInstance = 0 ) = 0;
  virtual void DrawIndexed( int indexCount, int instanceCount = 1, int startIndex = 0, int baseVertex = 0, int startInstance = 0 ) = 0;
  
  virtual void SetComputeShader( ComputeShader& shader ) = 0;
  virtual void SetComputeConstantValues( int index, const void* values, int numValues, int offset = 0 ) = 0;
  virtual void SetComputeConstantBuffer( int index, Resource& bufferResource ) = 0;
  virtual void SetComputeResource( int index, ResourceDescriptor& descriptor ) = 0;
  virtual void SetComputeRayTracingScene( int index, RTTopLevelAccelerator& accelerator ) = 0;
  virtual void SetComputeTextureHeap( int index, DescriptorHeap& heap, int offset ) = 0;

  virtual void SetVariableRateShading( VRSBlock block ) = 0;

  virtual void Dispatch( int groupsX, int groupsY, int groupsZ ) = 0;

  virtual void GenerateMipmaps( Resource& resource ) = 0;

  virtual void AddUAVBarrier( std::initializer_list< Resource* > resources ) = 0;

  virtual void DearImGuiRender( Device& device ) = 0;

  virtual void UploadTextureResource( std::unique_ptr< Resource > source, Resource& destination, const void* data, int stride, int rows ) = 0;
  virtual void UploadBufferResource( std::unique_ptr< Resource > source, Resource& destination, const void* data, int dataSize ) = 0;

  virtual void CopyResource( Resource& source, Resource& destination ) = 0;

  virtual void HoldResource( std::unique_ptr< Resource > resource ) = 0;

  virtual std::vector< std::unique_ptr< Resource > > TakeHeldResources() = 0;

  virtual void BeginEvent( int eventId, const wchar_t* format, ... ) = 0;
  virtual void EndEvent() = 0;

  using EndFrameCallback = std::function< void() >;
  virtual void RegisterEndFrameCallback( EndFrameCallback&& callback ) = 0;
  virtual std::vector< EndFrameCallback > TakeEndFrameCallbacks() = 0;

  template< typename T >
  void SetConstantValues( int index, const T& values )
  {
    static_assert( sizeof( values ) % 4 == 0, "Passed value should be 4 byte values only!" );
    SetConstantValues( index, &values, sizeof( values ) / 4, 0 );
  }

  template< typename T >
  void SetComputeConstantValues( int index, const T& values )
  {
    static_assert( sizeof( values ) % 4 == 0, "Passed value should be 4 byte values only!" );
    SetComputeConstantValues( index, &values, sizeof( values ) / 4, 0 );
  }

  void AddUAVBarrier( Resource& resource )
  {
    AddUAVBarrier( { &resource } );
  }
};