#pragma once

#include "../CommandQueue.h"
#include "../Types.h"

struct Fence;

class D3DCommandQueue : public CommandQueue
{
  friend class D3DDevice;

public:
  ~D3DCommandQueue();

  CommandQueueType GetCommandListType() const override;
  
  uint64_t IncrementFence() override;
  uint64_t GetNextFenceValue() override;
  uint64_t GetLastCompletedFenceValue() override;

  uint64_t Submit( CommandList& commandList ) override;

  bool IsFenceComplete( uint64_t fenceValue ) override;
  
  void WaitForFence( uint64_t fenceValue ) override;
  void WaitForIdle() override;

  ID3D12CommandQueue* GetD3DCommandQueue();

private:
  D3DCommandQueue( D3DDevice& device, CommandQueueType type );

  CommandQueueType              type;
  CComPtr< ID3D12CommandQueue > d3dCommandQueue;
  CComPtr< ID3D12Fence1 >       d3dFence;

  uint64_t nextFenceValue          = 1;
  uint64_t lastCompletedFenceValue = 0;

  std::mutex fenceMutex;
  std::mutex eventMutex;

  HANDLE fenceEventHandle;
};
