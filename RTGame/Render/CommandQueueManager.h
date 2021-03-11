#pragma once

#include "Types.h"

struct Device;
struct CommandQueue;

class CommandAllocatorPool;

class CommandQueueManager
{
  friend class CommandContext;

public:
  CommandQueueManager( Device& device );
  ~CommandQueueManager();

  CommandQueue&         GetQueue( CommandQueueType queueType );
  CommandAllocatorPool& GetAllocatorPool( CommandQueueType queueType );

  void IdleGPU();

private:
  std::unique_ptr< CommandQueue > graphicsQueue;
  std::unique_ptr< CommandQueue > computeQueue;
  std::unique_ptr< CommandQueue > copyQueue;

  std::unique_ptr< CommandAllocatorPool > graphicsCommandAllocatorPool;
  std::unique_ptr< CommandAllocatorPool > computeCommandAllocatorPool;
  std::unique_ptr< CommandAllocatorPool > copyCommandAllocatorPool;
};