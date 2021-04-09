#include "D3DCommandQueue.h"
#include "D3DCommandList.h"
#include "D3DDevice.h"
#include "Conversion.h"

D3DCommandQueue::D3DCommandQueue( D3DDevice& device, CommandQueueType type )
  : type( type )
{
  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type     = Convert( type );
  desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
  desc.NodeMask = 0;

  device.GetD3DDevice()->CreateCommandQueue( &desc, IID_PPV_ARGS( &d3dCommandQueue ) );

  device.GetD3DDevice()->CreateFence( 0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS( &d3dFence ) );
  d3dFence->SetName( L"D3DCommandQueue::d3dFence" );
  d3dFence->Signal( 0 );

  fenceEventHandle = CreateEvent( nullptr, false, false, nullptr );
}

D3DCommandQueue::~D3DCommandQueue()
{
  CloseHandle( fenceEventHandle );
}

ID3D12CommandQueue* D3DCommandQueue::GetD3DCommandQueue()
{
  return d3dCommandQueue;
}

CommandQueueType D3DCommandQueue::GetCommandListType() const
{
  return type;
}

void D3DCommandQueue::WaitForFence( uint64_t fenceValue )
{
  if ( IsFenceComplete( fenceValue ) )
    return;

  auto waitStart = GetCPUTime();

  std::lock_guard< std::mutex > lockGuard( eventMutex );

  d3dFence->SetEventOnCompletion( fenceValue, fenceEventHandle );
  WaitForSingleObject( fenceEventHandle, INFINITE );
  lastCompletedFenceValue = fenceValue;

  auto waitEnd = GetCPUTime();
  if ( waitEnd - waitStart > 0.1 )
  {
    char msg[ 128 ];
    sprintf_s( msg, "D3DCommandQueue::WaitForFence took %f seconds\n", waitEnd - waitStart );
    OutputDebugStringA( msg );
  }
}

void D3DCommandQueue::WaitForIdle()
{
  WaitForFence( IncrementFence() );
}

uint64_t D3DCommandQueue::GetFrequency()
{
  UINT64 frequency;
  if FAILED( d3dCommandQueue->GetTimestampFrequency( &frequency ) )
    frequency = 1;

  return frequency;
}

uint64_t D3DCommandQueue::IncrementFence()
{
  std::lock_guard< std::mutex > lockGuard( fenceMutex );
  d3dCommandQueue->Signal( d3dFence, nextFenceValue );
  return nextFenceValue++;
}

uint64_t D3DCommandQueue::GetNextFenceValue()
{
  return nextFenceValue;
}

uint64_t D3DCommandQueue::GetLastCompletedFenceValue()
{
  return lastCompletedFenceValue;
}

uint64_t D3DCommandQueue::Submit( CommandList& commandList )
{
  std::lock_guard< std::mutex > lockGuard( fenceMutex );

  auto d3dGraphicsCommandList = static_cast< D3DCommandList* >( &commandList )->GetD3DGraphicsCommandList();
  if ( d3dGraphicsCommandList )
  {
    auto hr = d3dGraphicsCommandList->Close();
    assert( SUCCEEDED( hr ) );
  }

  d3dCommandQueue->ExecuteCommandLists( 1, (ID3D12CommandList**)&d3dGraphicsCommandList );

  d3dCommandQueue->Signal( d3dFence, nextFenceValue );

  return nextFenceValue++;
}

bool D3DCommandQueue::IsFenceComplete( uint64_t fenceValue )
{
  if ( fenceValue > lastCompletedFenceValue )
    lastCompletedFenceValue = std::max( lastCompletedFenceValue, d3dFence->GetCompletedValue() );

  return fenceValue <= lastCompletedFenceValue;
}
