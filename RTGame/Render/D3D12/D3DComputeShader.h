#pragma once

#include "../ComputeShader.h"

class D3DComputeShader : public ComputeShader
{
  friend class D3DDevice;

public:
  ID3D12RootSignature* GetD3DRootSignature();
  ID3D12PipelineState* GetD3DPipelineState();

private:
  D3DComputeShader( D3DDevice& device, const void* shaderData, int shaderSize, const wchar_t* debugName );

  CComPtr< ID3D12RootSignature > d3dRootSignature;
  CComPtr< ID3D12PipelineState > d3dPipelineState;
};