#pragma once

#include "../PipelineState.h"
#include "../Types.h"

class D3DPipelineState : public PipelineState
{
  friend class D3DDevice;

public:
  ~D3DPipelineState();

  ID3D12PipelineState* GetD3DPipelineState();
  ID3D12RootSignature* GetD3DRootSignature();

private:
  D3DPipelineState( const PipelineDesc& desc, D3DDevice& device );

  CComPtr< ID3D12PipelineState > d3dPipelineState;
  CComPtr< ID3D12RootSignature > d3dRootSignature;
};