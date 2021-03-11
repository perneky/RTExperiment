#pragma once

#include "../Factory.h"

class D3DFactory : public Factory
{
  friend struct Factory;

public:
  ~D3DFactory();

  IDXGIFactoryX* GetDXGIFactory();

  bool IsVRRSupported() const override;

  std::unique_ptr< Adapter >   CreateDefaultAdapter() override;
  std::unique_ptr< Swapchain > CreateSwapchain( Device& device, CommandQueue& commandQueue, Window& window ) override;

private:
  D3DFactory();

  CComPtr< IDXGIFactoryX > dxgiFactory;
  bool                     variableRefreshRateSupported = false;
};
