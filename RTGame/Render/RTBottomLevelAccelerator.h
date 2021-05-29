#pragma once

struct Device;
struct CommandList;
struct Resource;

struct RTBottomLevelAccelerator
{
  virtual ~RTBottomLevelAccelerator() = default;

  virtual void Update( Device& device, CommandList& commandList, Resource& vertexBuffer, Resource& indexBuffer ) = 0;

  virtual int GetInfoIndex() const = 0;
};