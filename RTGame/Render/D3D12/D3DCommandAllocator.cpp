#include "D3DCommandAllocator.h"
#include "D3DDevice.h"
#include "D3DCommandQueue.h"
#include "Conversion.h"

D3DCommandAllocator::D3DCommandAllocator( D3DDevice& device, CommandQueueType& queueType )
{
  device.GetD3DDevice()->CreateCommandAllocator( Convert( queueType ), IID_PPV_ARGS( &d3dCommandAllocator ) );
}

D3DCommandAllocator::~D3DCommandAllocator()
{
}

void D3DCommandAllocator::Reset()
{
  d3dCommandAllocator->Reset();
}

ID3D12CommandAllocator* D3DCommandAllocator::GetD3DCommandAllocator()
{
  return d3dCommandAllocator;
}
