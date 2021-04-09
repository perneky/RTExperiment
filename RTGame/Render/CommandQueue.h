#pragma once

#include "Types.h"

struct CommandList;

struct CommandQueue
{
  virtual ~CommandQueue() = default;

  virtual CommandQueueType GetCommandListType() const = 0;

  virtual uint64_t IncrementFence() = 0;
  virtual uint64_t GetNextFenceValue() = 0;
  virtual uint64_t GetLastCompletedFenceValue() = 0;

  virtual uint64_t Submit( CommandList& commandList ) = 0;
  
  virtual bool IsFenceComplete( uint64_t fenceValue ) = 0;
  
  virtual void WaitForFence( uint64_t fenceValue ) = 0;
  virtual void WaitForIdle() = 0;

  virtual uint64_t GetFrequency() = 0;
};
