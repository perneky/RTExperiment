#include "D3DRTTopLevelAccelerator.h"
#include "D3DRTBottomLevelAccelerator.h"
#include "D3DResource.h"
#include "D3DCommandList.h"
#include "D3DDevice.h"
#include "D3DUtils.h"
#include "../ShaderValues.h"

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

D3DRTTopLevelAccelerator::D3DRTTopLevelAccelerator( D3DDevice& device, D3DCommandList& commandList, std::vector< RTInstance > instances )
{
  auto instanceDataSize = std::max( sizeof( D3D12_RAYTRACING_INSTANCE_DESC ) * instances.size(), size_t( 1 ) );

  AllocatedResource instanceDescs = AllocateUploadBuffer( device, instanceDataSize, L"InstanceDescs" );

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
  instanceDescs->Unmap( 0, nullptr );

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
  topLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
  topLevelInputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
  topLevelInputs.NumDescs       = UINT( instances.size() );
  topLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  topLevelInputs.InstanceDescs  = instanceDescs->GetGPUVirtualAddress();

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
  device.GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &topLevelInputs, &topLevelPrebuildInfo );
  assert( topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0 );

  CComPtr< ID3D12Resource > d3dScratchBuffer = device.RequestD3DRTScartchBuffer( commandList, int( topLevelPrebuildInfo.ScratchDataSizeInBytes ) );
  d3dUAVBuffer = AllocateUAVBuffer( device, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, ResourceStateBits::RTAccelerationStructure, L"TLAS" );

  ZeroMemory( &d3dAcceleratorDesc, sizeof( d3dAcceleratorDesc ) );
  d3dAcceleratorDesc.Inputs                           = topLevelInputs;
  d3dAcceleratorDesc.ScratchAccelerationStructureData = d3dScratchBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.DestAccelerationStructureData    = d3dUAVBuffer->GetGPUVirtualAddress();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.Transition.pResource   = *d3dUAVBuffer;

  commandList.GetD3DGraphicsCommandList()->BuildRaytracingAccelerationStructure( &d3dAcceleratorDesc, 0, nullptr );
  commandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );

  commandList.HoldResource( std::unique_ptr< Resource >( new D3DResource( std::move( instanceDescs ), ResourceStateBits::Common ) ) );
}

D3DRTTopLevelAccelerator::~D3DRTTopLevelAccelerator()
{
}

void D3DRTTopLevelAccelerator::Update( Device& device, CommandList& commandList, std::vector< RTInstance > instances )
{
  auto& d3dDevice        = *static_cast< D3DDevice* >( &device );
  auto& d3dCommandList   = *static_cast< D3DCommandList* >( &commandList );
  auto  instanceDataSize = sizeof( D3D12_RAYTRACING_INSTANCE_DESC ) * instances.size();

  AllocatedResource instanceDescs = AllocateUploadBuffer( d3dDevice, instanceDataSize, L"InstanceDescs" );

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
    mappedData->Flags	                              = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
    mappedData->AccelerationStructure               = static_cast< D3DRTBottomLevelAccelerator* >( instance.accel )->GetD3DUAVBuffer()->GetGPUVirtualAddress();

    mappedData++;
  }
  instanceDescs->Unmap( 0, nullptr );

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
  topLevelInputs.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
  topLevelInputs.Flags          = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  topLevelInputs.NumDescs       = UINT( instances.size() );
  topLevelInputs.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  topLevelInputs.InstanceDescs  = instanceDescs->GetGPUVirtualAddress();

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo = {};
  d3dDevice.GetD3DDevice()->GetRaytracingAccelerationStructurePrebuildInfo( &topLevelInputs, &topLevelPrebuildInfo );
  assert( topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0 );

  CComPtr< ID3D12Resource > d3dScratchBuffer = d3dDevice.RequestD3DRTScartchBuffer( d3dCommandList, int( topLevelPrebuildInfo.ScratchDataSizeInBytes ) );
  
  AllocatedResource d3dNewUAVBuffer;
  if ( d3dUAVBuffer->GetDesc().Width < topLevelPrebuildInfo.ResultDataMaxSizeInBytes )
    d3dNewUAVBuffer = AllocateUAVBuffer( d3dDevice, topLevelPrebuildInfo.ResultDataMaxSizeInBytes, ResourceStateBits::RTAccelerationStructure, L"TLAS" );

  d3dAcceleratorDesc.Inputs                           = topLevelInputs;
  d3dAcceleratorDesc.ScratchAccelerationStructureData = d3dScratchBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.SourceAccelerationStructureData  = d3dUAVBuffer->GetGPUVirtualAddress();
  d3dAcceleratorDesc.DestAccelerationStructureData    = d3dNewUAVBuffer ? d3dNewUAVBuffer->GetGPUVirtualAddress() : d3dUAVBuffer->GetGPUVirtualAddress();

  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
  barrier.Transition.pResource   = *d3dUAVBuffer;
  d3dCommandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );

  d3dCommandList.GetD3DGraphicsCommandList()->BuildRaytracingAccelerationStructure( &d3dAcceleratorDesc, 0, nullptr );

  barrier.Transition.pResource = d3dNewUAVBuffer ? *d3dNewUAVBuffer : *d3dUAVBuffer;
  d3dCommandList.GetD3DGraphicsCommandList()->ResourceBarrier( 1, &barrier );

  commandList.HoldResource( std::unique_ptr< Resource >( new D3DResource( std::move( instanceDescs ), ResourceStateBits::Common ) ) );

  if ( d3dNewUAVBuffer )
  {
    commandList.HoldResource( std::unique_ptr< Resource >( new D3DResource( std::move( d3dUAVBuffer ), ResourceStateBits::RTAccelerationStructure ) ) );
    d3dUAVBuffer = std::move( d3dNewUAVBuffer );
  }
}

ID3D12Resource* D3DRTTopLevelAccelerator::GetD3DUAVBuffer()
{
  return *d3dUAVBuffer;
}
