#pragma once

#include "../GPUTimeQuery.h"

class D3DDevice;
class D3DResource;

class D3DGPUTimeQuery : public GPUTimeQuery
{
public:
  D3DGPUTimeQuery( D3DDevice& device );
  ~D3DGPUTimeQuery();

  void   Insert( CommandList& commandList ) override;
  double GetResult( CommandList& commandList ) override;

private:
  std::unique_ptr< D3DResource > resultBuffer;

  CComPtr< ID3D12QueryHeap > d3dQueryHeap;

  bool   gotResult = false;
  double result    = -1;
};