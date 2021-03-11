#pragma once

#include "../Adapter.h"

class D3DAdapter : public Adapter
{
  friend class D3DFactory;

public:
  ~D3DAdapter();

  std::unique_ptr< Device > CreateDevice() override;

  IDXGIAdapter4* GetDXGIAdapter();

private:
  D3DAdapter( D3DFactory& factory );

  CComPtr< IDXGIAdapter4 > dxgiAdapter;
};
