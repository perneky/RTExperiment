#pragma once

struct Device;

struct Adapter
{
  virtual ~Adapter() = default;

  virtual std::unique_ptr< Device > CreateDevice() = 0;
};
