#include "CommandQueueManager.h"
#include "CommandQueue.h"
#include "Device.h"
#include "CommandAllocatorPool.h"

CommandQueueManager::CommandQueueManager( Device& device )
  : graphicsQueue( device.CreateCommandQueue( CommandQueueType::Direct ) )
  , computeQueue( device.CreateCommandQueue( CommandQueueType::Compute ) )
  , copyQueue( device.CreateCommandQueue( CommandQueueType::Copy ) )
  , graphicsCommandAllocatorPool( new CommandAllocatorPool( CommandQueueType::Direct ) )
  , computeCommandAllocatorPool( new CommandAllocatorPool( CommandQueueType::Compute ) )
  , copyCommandAllocatorPool( new CommandAllocatorPool( CommandQueueType::Copy ) )
{
}

CommandQueueManager::~CommandQueueManager()
{
  IdleGPU();
}

CommandQueue& CommandQueueManager::GetQueue( CommandQueueType queueType )
{
  switch ( queueType )
  {
  case CommandQueueType::Direct:  return *graphicsQueue;
  case CommandQueueType::Compute: return *computeQueue;
  case CommandQueueType::Copy:    return *copyQueue;
  }

  assert( false );
  return *graphicsQueue;
}

CommandAllocatorPool& CommandQueueManager::GetAllocatorPool( CommandQueueType queueType )
{
  switch ( queueType )
  {
  case CommandQueueType::Direct:  return *graphicsCommandAllocatorPool;
  case CommandQueueType::Compute: return *computeCommandAllocatorPool;
  case CommandQueueType::Copy:    return *copyCommandAllocatorPool;
  }

  assert( false );
  return *graphicsCommandAllocatorPool;
}

void CommandQueueManager::IdleGPU()
{
  graphicsQueue->WaitForIdle();
  computeQueue->WaitForIdle();
  copyQueue->WaitForIdle();
}
