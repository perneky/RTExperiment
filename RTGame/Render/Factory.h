#pragma once

struct Adapter;
struct Device;
struct Swapchain;
struct CommandQueue;
struct Window;

struct Factory
{
  static std::unique_ptr< Factory > Create();

  virtual ~Factory() = default;

  virtual bool IsVRRSupported() const = 0;

  virtual std::unique_ptr< Adapter >   CreateDefaultAdapter() = 0;
  virtual std::unique_ptr< Swapchain > CreateSwapchain( Device& device, CommandQueue& commandQueue, Window& window ) = 0;
};
