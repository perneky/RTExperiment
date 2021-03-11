#pragma once

struct ResourceDescriptor
{
  virtual ~ResourceDescriptor() = default;

  virtual int GetSlot() const = 0;
};