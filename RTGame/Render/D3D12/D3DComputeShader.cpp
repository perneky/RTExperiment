#include "D3DComputeShader.h"
#include "D3DDevice.h"

D3DComputeShader::D3DComputeShader( D3DDevice& device, const void* shaderData, int shaderSize, const wchar_t* debugName )
{
  auto hr = device.GetD3DDevice()->CreateRootSignature( 0, shaderData, shaderSize, IID_PPV_ARGS( &d3dRootSignature ) );
  assert( SUCCEEDED( hr ) );

  D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
  desc.pRootSignature     = d3dRootSignature;
  desc.CS.pShaderBytecode = shaderData;
  desc.CS.BytecodeLength  = SIZE_T( shaderSize );
  device.GetD3DDevice()->CreateComputePipelineState( &desc, IID_PPV_ARGS( &d3dPipelineState ) );
  if ( debugName )
    d3dPipelineState->SetName( debugName );
}

ID3D12RootSignature* D3DComputeShader::GetD3DRootSignature()
{
  return d3dRootSignature;
}

ID3D12PipelineState* D3DComputeShader::GetD3DPipelineState()
{
  return d3dPipelineState;
}
