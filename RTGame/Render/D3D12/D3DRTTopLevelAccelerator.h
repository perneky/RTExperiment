#pragma once

#include "../RTTopLevelAccelerator.h"
#include "../Types.h"

class D3DCommandList;
class D3DResource;
class D3DRTBottomLevelAccelerator;
class D3DDescriptorHeap;

class D3DRTTopLevelAccelerator : public RTTopLevelAccelerator
{
  friend class D3DDevice;

public:
  virtual ~D3DRTTopLevelAccelerator();

  void Update( Device& device, CommandList& commandList, std::vector< RTInstance > instances ) override;

  ID3D12Resource2* GetD3DUAVBuffer();

private:
  D3DRTTopLevelAccelerator( D3DDevice& device, D3DCommandList& commandList, std::vector< RTInstance > instances );

  CComPtr< ID3D12Resource2 >                         d3dUAVBuffer;
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3dAcceleratorDesc;
};