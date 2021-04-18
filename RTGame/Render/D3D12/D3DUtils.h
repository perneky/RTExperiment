#pragma once

#include "AllocatedResource.h"
#include "Conversion.h"

static AllocatedResource AllocateUAVBuffer( D3DDevice& device, UINT64 bufferSize, ResourceState initialResourceState = ResourceStateBits::Common, const wchar_t* resourceName = nullptr )
{
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );
  auto allocation = device.AllocateResource( HeapType::Default, bufferDesc, initialResourceState );
  if ( resourceName )
    allocation->SetName( resourceName );
  return allocation;
}

static AllocatedResource AllocateUploadBuffer( D3DDevice& device, UINT64 datasize, const wchar_t* resourceName = nullptr )
{
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( datasize );
  auto allocation = device.AllocateResource( HeapType::Upload, bufferDesc, ResourceStateBits::GenericRead );
  if ( resourceName )
    allocation->SetName( resourceName );
  return allocation;
}
