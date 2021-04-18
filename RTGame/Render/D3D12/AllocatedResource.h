#pragma once

#include "../D3D12MemoryAllocator/D3D12MemAlloc.h"

struct AllocatedResource
{
  AllocatedResource() noexcept = default;
  explicit AllocatedResource( D3D12MA::Allocation* allocation ) noexcept : allocation( allocation ) {}
  explicit AllocatedResource( ID3D12Resource* d3dResource ) noexcept
    : d3dResource( d3dResource ) 
  {
    if ( d3dResource )
      d3dResource->AddRef();
  }

  AllocatedResource( AllocatedResource&& other ) noexcept
    : allocation( other.allocation )
    , d3dResource( other.d3dResource )
  {
    other.allocation  = nullptr;
    other.d3dResource = nullptr;
  }
  AllocatedResource& operator = ( AllocatedResource&& other )
  {
    Release();
    allocation        = other.allocation;
    d3dResource       = other.d3dResource;
    other.allocation  = nullptr;
    other.d3dResource = nullptr;
    return *this;
  }

  AllocatedResource( const AllocatedResource& ) noexcept = delete;
  AllocatedResource& operator = ( AllocatedResource& other ) noexcept = delete;

  ~AllocatedResource()
  {
    Release();
  }

  ID3D12Resource* operator -> () const
  {
    assert( allocation || d3dResource );
    return allocation ? allocation->GetResource() : d3dResource;
  }

  ID3D12Resource* operator * () const noexcept
  {
    assert( allocation || d3dResource );
    return allocation ? allocation->GetResource() : d3dResource;
  }

  operator bool() const
  {
    return !!allocation || !!d3dResource;
  }

  void Release()
  {
    if ( allocation )
    {
      allocation->Release();
      allocation = nullptr;
    }
    else if ( d3dResource )
    {
      d3dResource->Release();
      d3dResource = nullptr;
    }
  }

private:
  D3D12MA::Allocation* allocation  = nullptr;
  ID3D12Resource*      d3dResource = nullptr;
};
