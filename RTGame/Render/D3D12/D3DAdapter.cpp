#include "D3DAdapter.h"
#include "D3DFactory.h"
#include "D3DDevice.h"

D3DAdapter::D3DAdapter( D3DFactory& factory )
{
  auto dxgiFactory = factory.GetDXGIFactory();

  CComPtr< IDXGIAdapter1 > adapter1;
  CComPtr< IDXGIAdapter4 > adapter4;

  auto maxDedicatedVideoMemory = 0LLU;
  for ( auto i = 0U; dxgiFactory->EnumAdapters1( i, &adapter1 ) != DXGI_ERROR_NOT_FOUND; ++i )
  {
    DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
    adapter1->GetDesc1( &dxgiAdapterDesc1 );

    if ( ( dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE ) == 0 )
    {
      if SUCCEEDED( D3D12CreateDevice( adapter1, D3D_FEATURE_LEVEL_12_1, __uuidof( ID3D12Device ), nullptr ) )
      {
        if ( dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory )
        {
          maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
          adapter1->QueryInterface( &adapter4 );
        }
      }
    }

    adapter1.Release();
  }

  dxgiAdapter = adapter4;
}

D3DAdapter::~D3DAdapter()
{
}

std::unique_ptr<Device> D3DAdapter::CreateDevice()
{
  return std::unique_ptr< Device >( new D3DDevice( *this ) );
}

IDXGIAdapter4* D3DAdapter::GetDXGIAdapter()
{
  return dxgiAdapter;
}
