#pragma once

struct CommandList;

struct CommandAllocator
{
  virtual ~CommandAllocator() = default;

  virtual void Reset() = 0;
};
