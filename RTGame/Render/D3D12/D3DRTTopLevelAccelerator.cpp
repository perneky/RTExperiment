#include "D3DRTTopLevelAccelerator.h"
#include "D3DRTBottomLevelAccelerator.h"
#include "D3DResource.h"
#include "D3DCommandList.h"
#include "D3DDevice.h"
#include "D3DUtils.h"
#include "../ShaderValues.h"

ID3D12Resource2*               D3DRTTopLevelAccelerator::aabbUAVBuffer  = nullptr;
std::unique_ptr< D3DResource > D3DRTTopLevelAccelerator::aabbUnitBuffer;

static UINT PackIBVBSlots( const RTInstance& instance )
{
  uint32_t hlslIBSlot = instance.rtIBSlot - CBVIBBaseSlot;
  uint32_t hlslVBSlot = instance.rtVBSlot - CBVVBBaseSlot;
  assert( hlslIBSlot <= RTSlotMask );
  assert( hlslVBSlot <= RTSlotMask );
  hlslIBSlot &= RTSlotMask;
  hlslVBSlot &= RTSlotMask;
  return ( hlslIBSlot << 12 ) | hlslVBSlot;
}

CComPtr< ID3D12Resource2 > D3DRTTopLevelAccelerator::FillTLASInstanceBuffer( D3DDevice& device, D3DCommandList& commandList, const std::vector< RTInstance >& instances, const std::vector< SubAccel >& areas )
{
  auto instanceDataSize = std::max( sizeof( D3D12_RAYTRACING_INSTANCE_DESC ) * ( instances.size() + areas.size() ), size_t( 1 ) );

  CComPtr< ID3D12Resource2 > instanceDescs = AllocateUploadBuffer( device.GetD3DDevice(), instanceDataSize, L"InstanceDescs" );

  D3D12_RAYTRACING_INSTANCE_DESC* mappedData;
  instanceDescs->Map( 0, nullptr, (void**)&mappedData );
  for ( auto& instance : instances )
  {
    mappedData->Transform[ 0 ][ 0 ]                 = instance.transform->_11;
    mappedData->Transform[ 0 ][ 1 ]                 = instance.transform->_21;
    mappedData->Transform[ 0 ][ 2 ]                 = instance.transform->_31;
    mappedData->Transform[ 0 ][ 3 ]                 = instance.transform->_41;
    mappedData->Transform[ 1 ][ 0 ]                 = instance.transform->_12;
    mappedData->Transform[ 1 ][ 1 ]                 = instance.transform->_22;
    mappedData->Transform[ 1 ][ 2 ]                 = instance.transform->_32;
    mappedData->Transform[ 1 ][ 3 ]                 = instance.transform->_42;
    mappedData->Transform[ 2 ][ 0 ]                 = instance.transform->_13;
    mappedData->Transform[ 2 ][ 1 ]                 = instance.transform->_23;
    mappedData->Transform[ 2 ][ 2 ]                 = instance.transform->_33;
    mappedData->Transform[ 2 ][ 3 ]                 = instance.transform->_43;
    mappedData->InstanceID                          = PackIBVBSlots( instance );
    mappedData->InstanceMask                        = 0xFFU;
    mappedData->InstanceContributionToHitGroupIndex = 0;
    mappedData->Flags	                              = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
    mappedData->AccelerationStructure               = static_cast< D3DRTBottomLevelAccelerator* >( instance.accel )->GetD3DUAVBuffer()->GetGPUVirtualAddress();

    mappedData++;
  }

  if ( !areas.empty() )
    AcquireAABBUAVBuffer( device, commandList );

  for ( auto& area : areas )
  {
    mappedData->Transform[ 0 ][ 0 ]                 = area.aabb.Extents.x;
    mappedData->Transform[ 0 ][ 1 ]                 = 0;
    mappedData->Transform[ 0 ][ 2 ]                 = 0;
    mappedData->Transform[ 0 ][ 3 ]                 = area.aabb.Center.x;
    mappedData->Transform[ 1 ][ 0 ]                 = 0;
    mappedData->Transform[ 1 ][ 1 ]                 = area.aabb.Extents.y;
    mappedData->Transform[ 1 ][ 2 ]                 = 0;
    mappedData->Transform[ 1 ][ 3 ]                 = area.aabb.Center.y;
    mappedData->Transform[ 2 ][ 0 ]                 = 0;
    mappedData->Transform[ 2 ][ 1 ]                 = 0;
    mappedData->Transform[ 2 ][ 2 ]                 = area.aabb.Extents.z;
    mappedData->Transform[ 2 ][ 3 ]                 = area.aabb.Center.z;
    mappedData->InstanceID                          = area.id;
    mappedData->InstanceMask                        = 0xFFU;
    mappedData->InstanceContributionToHitGroupIndex = 0;
    mappedData->Flags	                              = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    mappedData->AccelerationStructure               = aabbUAVBuffer->GetGPUVirtualAddress();

    mappedData++;
  }
  instanceDescs->Unmap( 0, nullptr );

  return instanceDescs;
}

D3DRTTopLevelAccelerator::D3DRTTopLevelAccelerator( D3DDevice& device, D3DCommandList& commandList, std::vector< RTInstance > instances, std::vector< SubAccel > areas )
{
  auto instanceDescs = FillTLASInstanceBuffer( device, commandList, instances, areas );

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
  topLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
  topLevelInputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
  topLevelInputs.NumDescs       = UINT( instances.size() + areas.size() );
  topLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  topLevelInputs.InstanceDescs  = instanceDescs->GetGPUVirtualAddress();

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
  device.GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &topLevelInputs, &topLevelPrebuildInfo );
  assert( topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0 );

  CComPtr< ID3D12Resource2 > d3dScratchBuffer = device.RequestD3DRTScartchBuffer( commandList, int( topLevelPrebuildInfo.ScratchDataSizeInBytes ) );
  d3dUAVBuffer = AllocateUAVBuffer( device.GetD3DDevice(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"TLAS" );

  d3dAcceleratorDesc.Inputs                           = topLevelInputs;
  d3dAcceleratorDesc.ScratchAccelerationStructureData = d3dScratchBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.DestAccelerationStructureData    = d3dUAVBuffer->GetGPUVirtualAddress();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.Transition.pResource   = d3dUAVBuffer;

  commandList.GetD3DGraphicsCommandList()->BuildRaytracingAccelerationStructure( &d3dAcceleratorDesc, 0, nullptr );
  commandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );

  commandList.HoldResource( std::unique_ptr< Resource >( new D3DResource( instanceDescs, ResourceStateBits::Common ) ) );
}

void D3DRTTopLevelAccelerator::AcquireAABBUAVBuffer( D3DDevice& device, D3DCommandList& commandList )
{
  if ( aabbUAVBuffer )
    return;

  D3D12_RAYTRACING_AABB mappedData;
  mappedData.MinX = mappedData.MinY = mappedData.MinZ = -1;
  mappedData.MaxX = mappedData.MaxY = mappedData.MaxZ =  1;

  auto aabbUnitUpload = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Upload, false, sizeof( D3D12_RAYTRACING_AABB ), 0, L"AABBUpload" );

  aabbUnitBuffer.reset( static_cast< D3DResource* >( device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, true, sizeof( D3D12_RAYTRACING_AABB ), 0, L"AABB" ).release() ) );

  commandList.UploadBufferResource( std::move( aabbUnitUpload ), *aabbUnitBuffer, &mappedData, sizeof( mappedData ) );
  commandList.ChangeResourceState( *aabbUnitBuffer, ResourceStateBits::NonPixelShaderInput );

  D3D12_RAYTRACING_GEOMETRY_DESC d3dGeometryDesc;

  d3dGeometryDesc.Flags                     = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
  d3dGeometryDesc.Type                      = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
  d3dGeometryDesc.AABBs.AABBCount           = 1;
  d3dGeometryDesc.AABBs.AABBs.StartAddress  = aabbUnitBuffer->GetD3DGPUVirtualAddress();
  d3dGeometryDesc.AABBs.AABBs.StrideInBytes = 0;

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
  bottomLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
  bottomLevelInputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  bottomLevelInputs.NumDescs       = 1;
  bottomLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  bottomLevelInputs.pGeometryDescs = &d3dGeometryDesc;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo = {};
  device.GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &bottomLevelInputs, &bottomLevelPrebuildInfo );
  assert( bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0 );

  CComPtr< ID3D12Resource2 > d3dScratchBuffer = device.RequestD3DRTScartchBuffer( commandList, int( bottomLevelPrebuildInfo.ScratchDataSizeInBytes ) );
  aabbUAVBuffer = AllocateUAVBuffer( device.GetD3DDevice(), bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"BLAS" );

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3dAcceleratorDesc;
  d3dAcceleratorDesc.Inputs                           = bottomLevelInputs;
  d3dAcceleratorDesc.ScratchAccelerationStructureData = d3dScratchBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.DestAccelerationStructureData    = aabbUAVBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.SourceAccelerationStructureData  = 0;

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.Transition.pResource   = aabbUAVBuffer;

  commandList.GetD3DGraphicsCommandList()->BuildRaytracingAccelerationStructure( &d3dAcceleratorDesc, 0, nullptr );
  commandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );
}

void D3DRTTopLevelAccelerator::ReleaseAABBUAVBuffer()
{
  // intentionally leaking aabbUnitBuffer and aabbUAVBuffer for now
}

D3DRTTopLevelAccelerator::~D3DRTTopLevelAccelerator()
{
  ReleaseAABBUAVBuffer();
}

bool D3DRTTopLevelAccelerator::Update( Device& device, CommandList& commandList, std::vector< RTInstance > instances, std::vector< SubAccel > areas )
{
  auto& d3dDevice      = *static_cast< D3DDevice* >( &device );
  auto& d3dCommandList = *static_cast< D3DCommandList* >( &commandList );

  ReleaseAABBUAVBuffer();

  auto instanceDescs = FillTLASInstanceBuffer( d3dDevice, d3dCommandList, instances, areas );

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
  topLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
  topLevelInputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  topLevelInputs.NumDescs       = UINT( instances.size() + areas.size() );
  topLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  topLevelInputs.InstanceDescs  = instanceDescs->GetGPUVirtualAddress();

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
  d3dDevice.GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &topLevelInputs, &topLevelPrebuildInfo );
  assert( topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0 );

  bool inplaceUpdate = true;

  CComPtr< ID3D12Resource2 > d3dScratchBuffer = d3dDevice.RequestD3DRTScartchBuffer( d3dCommandList, int( topLevelPrebuildInfo.ScratchDataSizeInBytes ) );
  auto d3dNewUAVBuffer = d3dUAVBuffer;
  if ( d3dUAVBuffer->GetDesc1().Width < topLevelPrebuildInfo.ResultDataMaxSizeInBytes )
  {
    d3dNewUAVBuffer = AllocateUAVBuffer( d3dDevice.GetD3DDevice(), topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"TLAS" );
    inplaceUpdate = false;
  }

  d3dAcceleratorDesc.Inputs                           = topLevelInputs;
  d3dAcceleratorDesc.ScratchAccelerationStructureData = d3dScratchBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.SourceAccelerationStructureData  = d3dUAVBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.DestAccelerationStructureData    = d3dNewUAVBuffer->GetGPUVirtualAddress();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.Transition.pResource   = d3dUAVBuffer;
  d3dCommandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );

  d3dCommandList.GetD3DGraphicsCommandList()->BuildRaytracingAccelerationStructure( &d3dAcceleratorDesc, 0, nullptr );

  barrier.Transition.pResource = d3dNewUAVBuffer;
  d3dCommandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );

  commandList.HoldResource( std::unique_ptr< Resource >( new D3DResource( instanceDescs, ResourceStateBits::Common ) ) );
  commandList.HoldResource( std::unique_ptr< Resource >( new D3DResource( d3dUAVBuffer, ResourceStateBits::RTAccelerationStructure ) ) );

  d3dUAVBuffer = d3dNewUAVBuffer;

  return !inplaceUpdate;
}

ID3D12Resource2* D3DRTTopLevelAccelerator::GetD3DUAVBuffer()
{
  return d3dUAVBuffer;
}
