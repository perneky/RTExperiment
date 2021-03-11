#pragma once

#include "../Types.h"
#include "../RTBottomLevelAccelerator.h"

class D3DCommandList;
class D3DResource;
class D3DDescriptorHeap;

class D3DRTBottomLevelAccelerator : public RTBottomLevelAccelerator
{
  friend class D3DDevice;

public:
  virtual ~D3DRTBottomLevelAccelerator();

  void Update( Device& device, CommandList& commandList, Resource& vertexBuffer, Resource& indexBuffer ) override;

  ID3D12Resource2* GetD3DUAVBuffer();

private:
  D3DRTBottomLevelAccelerator( D3DDevice& device, D3DCommandList& commandList, D3DResource& vertexBuffer, int vertexCount, int positionElementSize, int vertexStride, D3DResource& indexBuffer, int indexSize, int indexCount, bool allowUpdate, bool fastBuild );

  CComPtr< ID3D12Resource2 >                         d3dUAVBuffer;
  D3D12_RAYTRACING_GEOMETRY_DESC                     d3dGeometryDesc;
  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC d3dAcceleratorDesc;
};