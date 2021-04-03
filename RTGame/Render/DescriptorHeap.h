#pragma once

#include "Types.h"

struct Device;
struct Resource;
struct ResourceDescriptor;
struct RTTopLevelAccelerator;

struct DescriptorHeap
{
  virtual ~DescriptorHeap() = default;

  virtual std::unique_ptr< ResourceDescriptor > RequestDescriptor( Device& device, ResourceDescriptorType type, int slot, Resource& resource, int bufferElementSize, int mipLevel = 0 ) = 0;
  virtual std::unique_ptr< ResourceDescriptor > RequestDescriptor( Device& device, int slot, RTTopLevelAccelerator& accel ) = 0;

  virtual int GetDescriptorSize() const = 0;
};