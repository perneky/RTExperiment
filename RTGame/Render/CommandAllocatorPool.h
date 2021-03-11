#pragma once

#include "Types.h"

struct Device;
struct CommandAllocator;

class CommandAllocatorPool
{
public:
  CommandAllocatorPool( CommandQueueType queueType );
  ~CommandAllocatorPool();

  CommandAllocator* RequestAllocator( Device& device, uint64_t completedFenceValue );
  void DiscardAllocator( uint64_t fenceValue, CommandAllocator* allocator );

  size_t Size() const;

private:
  CommandQueueType commandQueueType;

  std::vector< std::unique_ptr< CommandAllocator > >     allocatorPool;
  std::queue< std::pair< uint64_t, CommandAllocator* > > readyAllocators;
  std::mutex                                             allocatorMutex;
};