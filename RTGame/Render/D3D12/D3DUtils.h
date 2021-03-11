#pragma once

static ID3D12Resource2* AllocateUAVBuffer( ID3D12Device* pDevice, UINT64 bufferSize, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr )
{
  ID3D12Resource2* d3dResource;
  auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS );
  pDevice->CreateCommittedResource( &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, initialResourceState, nullptr, IID_PPV_ARGS( &d3dResource ) );
  if ( resourceName )
    d3dResource->SetName( resourceName );
  return d3dResource;
}

static ID3D12Resource2* AllocateUploadBuffer( ID3D12Device* pDevice, UINT64 datasize, const wchar_t* resourceName = nullptr )
{
  ID3D12Resource2* d3dResource;
  auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
  auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( datasize );
  pDevice->CreateCommittedResource( &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS( &d3dResource ) );
  if ( resourceName )
    d3dResource->SetName( resourceName );
  return d3dResource;
}
