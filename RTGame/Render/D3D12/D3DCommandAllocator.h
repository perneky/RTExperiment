#pragma once

#include "../CommandAllocator.h"
#include "../Types.h"

class D3DCommandAllocator : public CommandAllocator
{
  friend class D3DDevice;

public:
  ~D3DCommandAllocator();

  void Reset() override;

  ID3D12CommandAllocator* GetD3DCommandAllocator();

private:
  D3DCommandAllocator( D3DDevice& device, CommandQueueType& queueType );

  CComPtr< ID3D12CommandAllocator > d3dCommandAllocator;
};
