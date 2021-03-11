#pragma once

struct Device;
struct Resource;
struct CommandList;
struct Window;

struct Swapchain
{
  virtual ~Swapchain() = default;

  virtual void BuildBackBufferTextures( Device& device ) = 0;

  virtual Resource& GetCurrentBackBufferTexture() = 0;

  virtual uint64_t Present( uint64_t fenceValue ) = 0;

  virtual void Resize( CommandList& commandList, Device& device, Window& window ) = 0;
};
