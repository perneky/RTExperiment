#include "D3DRTBottomLevelAccelerator.h"
#include "D3DResource.h"
#include "D3DCommandList.h"
#include "D3DDevice.h"
#include "D3DDescriptorHeap.h"
#include "D3DUtils.h"

D3DRTBottomLevelAccelerator::D3DRTBottomLevelAccelerator( D3DDevice& device, D3DCommandList& commandList, D3DResource& vertexBuffer, int vertexCount, int positionElementSize, int vertexStride, D3DResource& indexBuffer, int indexSize, int indexCount, int infoIndex, bool allowUpdate, bool fastBuild )
{
  assert( positionElementSize == 32 || positionElementSize == 16 );
  assert( indexSize == 32 || indexSize == 16 );

  this->infoIndex = infoIndex;

  d3dGeometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
  d3dGeometryDesc.Triangles.IndexBuffer                = indexBuffer.GetD3DGPUVirtualAddress();
  d3dGeometryDesc.Triangles.IndexCount                 = indexCount;
  d3dGeometryDesc.Triangles.IndexFormat                = indexSize == 32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
  d3dGeometryDesc.Triangles.Transform3x4               = 0;
  d3dGeometryDesc.Triangles.VertexFormat               = positionElementSize == 32 ? DXGI_FORMAT_R32G32B32_FLOAT : DXGI_FORMAT_R16G16B16A16_FLOAT;
  d3dGeometryDesc.Triangles.VertexCount                = vertexCount;
  d3dGeometryDesc.Triangles.VertexBuffer.StartAddress  = vertexBuffer.GetD3DGPUVirtualAddress();
  d3dGeometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexStride;
  d3dGeometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
  if ( allowUpdate )
    flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
  if ( fastBuild )
    flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
  else
    flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
  bottomLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
  bottomLevelInputs.Flags          = flags;
  bottomLevelInputs.NumDescs       = 1;
  bottomLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  bottomLevelInputs.pGeometryDescs = &d3dGeometryDesc;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
  device.GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &bottomLevelInputs, &bottomLevelPrebuildInfo );
  assert( bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0 );

  CComPtr< ID3D12Resource > d3dScratchBuffer = device.RequestD3DRTScartchBuffer( commandList, int( bottomLevelPrebuildInfo.ScratchDataSizeInBytes ) );
  d3dUAVBuffer = AllocateUAVBuffer( device, bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, ResourceStateBits::RTAccelerationStructure, L"BLAS" );

  ZeroMemory( &d3dAcceleratorDesc, sizeof( d3dAcceleratorDesc ) );
  d3dAcceleratorDesc.Inputs                           = bottomLevelInputs;
  d3dAcceleratorDesc.ScratchAccelerationStructureData = d3dScratchBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.DestAccelerationStructureData    = d3dUAVBuffer->GetGPUVirtualAddress();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.Transition.pResource   = *d3dUAVBuffer;

  commandList.GetD3DGraphicsCommandList()->BuildRaytracingAccelerationStructure( &d3dAcceleratorDesc, 0, nullptr );
  commandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );
}

D3DRTBottomLevelAccelerator::~D3DRTBottomLevelAccelerator()
{
}

void D3DRTBottomLevelAccelerator::Update( Device& device, CommandList& commandList, Resource& vertexBuffer, Resource& indexBuffer )
{
  assert( d3dGeometryDesc.Triangles.IndexBuffer               == static_cast< D3DResource* >( &indexBuffer  )->GetD3DGPUVirtualAddress() );
  assert( d3dGeometryDesc.Triangles.VertexBuffer.StartAddress == static_cast< D3DResource* >( &vertexBuffer )->GetD3DGPUVirtualAddress() );

  auto& d3dDevice      = *static_cast< D3DDevice* >( &device );
  auto& d3dCommandList = *static_cast< D3DCommandList* >( &commandList );

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
  d3dDevice.GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &d3dAcceleratorDesc.Inputs, &bottomLevelPrebuildInfo );
  assert( bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0 );

  CComPtr< ID3D12Resource > d3dScratchBuffer = d3dDevice.RequestD3DRTScartchBuffer( d3dCommandList, int( bottomLevelPrebuildInfo.ScratchDataSizeInBytes ) );

  d3dAcceleratorDesc.Inputs.Flags                    |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  d3dAcceleratorDesc.ScratchAccelerationStructureData = d3dScratchBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.DestAccelerationStructureData    = d3dUAVBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.SourceAccelerationStructureData  = d3dUAVBuffer->GetGPUVirtualAddress();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.Transition.pResource   = *d3dUAVBuffer;
  d3dCommandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );

  d3dCommandList.GetD3DGraphicsCommandList()->BuildRaytracingAccelerationStructure( &d3dAcceleratorDesc, 0, nullptr );

  barrier.Transition.pResource = *d3dUAVBuffer;
  d3dCommandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );
}

int D3DRTBottomLevelAccelerator::GetInfoIndex() const
{
  return infoIndex;
}

ID3D12Resource* D3DRTBottomLevelAccelerator::GetD3DUAVBuffer()
{
  return *d3dUAVBuffer;
}
