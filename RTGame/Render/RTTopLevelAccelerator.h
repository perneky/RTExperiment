#pragma once

#include "Types.h"

struct RTInstance;
struct CommandList;
struct Device;

struct RTTopLevelAccelerator
{
  virtual ~RTTopLevelAccelerator() = default;

  virtual bool Update( Device& device, CommandList& commandList, std::vector< RTInstance > instances, std::vector< SubAccel > areas ) = 0;
};