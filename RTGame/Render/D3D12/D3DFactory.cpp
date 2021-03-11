#include "D3DFactory.h"
#include "D3DAdapter.h"
#include "D3DDevice.h"
#include "D3DSwapchain.h"
#include "D3DCommandQueue.h"
#include "Platform/Windows/WinAPIWindow.h"

std::unique_ptr< Factory > Factory::Create()
{
  return std::unique_ptr< Factory >( new D3DFactory );
}

D3DFactory::D3DFactory()
{
  UINT createFactoryFlags = 0;

#if DEBUG_GFX_API
  createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

  CComPtr< ID3D12Debug > debugInterface;
  D3D12GetDebugInterface( IID_PPV_ARGS( &debugInterface ) );
  debugInterface->EnableDebugLayer();
#endif // DEBUG_GFX_API

  BOOL allowTearing = FALSE;

  if SUCCEEDED( CreateDXGIFactory2( createFactoryFlags, IID_PPV_ARGS( &dxgiFactory ) ) )
    if FAILED( dxgiFactory->CheckFeatureSupport( DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof( allowTearing ) ) )
      allowTearing = FALSE;

  variableRefreshRateSupported = allowTearing != FALSE;
}

D3DFactory::~D3DFactory()
{
}
IDXGIFactoryX* D3DFactory::GetDXGIFactory()
{
  return dxgiFactory;
}

bool D3DFactory::IsVRRSupported() const
{
  return variableRefreshRateSupported;
}

std::unique_ptr< Adapter > D3DFactory::CreateDefaultAdapter()
{
  return std::unique_ptr< Adapter >( new D3DAdapter( *this ) );
}

std::unique_ptr< Swapchain > D3DFactory::CreateSwapchain( Device& device, CommandQueue& commandQueue, Window& window )
{
  return std::unique_ptr< Swapchain >( new D3DSwapchain( *this
                                                       , *static_cast< D3DCommandQueue* >( &commandQueue )
                                                       , *static_cast< D3DDevice* >( &device )
                                                       , *static_cast< WinAPIWindow* >( &window ) ) );
}
