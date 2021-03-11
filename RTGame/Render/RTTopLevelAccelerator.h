#pragma once

struct RTInstance;
struct CommandList;
struct Device;

struct RTTopLevelAccelerator
{
  virtual ~RTTopLevelAccelerator() = default;

  virtual void Update( Device& device, CommandList& commandList, std::vector< RTInstance > instances ) = 0;
};