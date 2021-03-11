#include "D3DPipelineState.h"
#include "D3DDevice.h"
#include "Conversion.h"

D3DPipelineState::D3DPipelineState( const PipelineDesc& desc, D3DDevice& device )
{
  auto d3dDevice = device.GetD3DDevice();
    
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
  ZeroObject( psoDesc );

  auto hr = d3dDevice->CreateRootSignature( 0, desc.vsData, desc.vsSize, IID_PPV_ARGS( &d3dRootSignature ) );
  assert( SUCCEEDED( hr ) );

  CComPtr< ID3D12RootSignatureDeserializer > deserializer;
  D3D12CreateRootSignatureDeserializer( desc.vsData, desc.vsSize, IID_PPV_ARGS( &deserializer ) );
  auto rsDesc = deserializer->GetRootSignatureDesc();

  psoDesc.pRootSignature = d3dRootSignature;

  psoDesc.VS.pShaderBytecode = desc.vsData;
  psoDesc.VS.BytecodeLength  = desc.vsSize;
  psoDesc.PS.pShaderBytecode = desc.psData;
  psoDesc.PS.BytecodeLength  = desc.psSize;

  int rtCounter = 0;
  for ( auto format : desc.targetFormat )
  {
    if ( format == PixelFormat::Unknown )
      break;

    psoDesc.RTVFormats[ rtCounter ] = Convert( format );
    rtCounter++;
  }

  psoDesc.NumRenderTargets = rtCounter;

  psoDesc.BlendState.AlphaToCoverageEnable                   = desc.blendDesc.alphaToCoverage;
  psoDesc.BlendState.IndependentBlendEnable                  = false;
  psoDesc.BlendState.RenderTarget[ 0 ].BlendEnable           = desc.blendDesc.alphaBlend;
  psoDesc.BlendState.RenderTarget[ 0 ].LogicOpEnable         = false;
  psoDesc.BlendState.RenderTarget[ 0 ].SrcBlend              = Convert( desc.blendDesc.sourceColorBlend );
  psoDesc.BlendState.RenderTarget[ 0 ].DestBlend             = Convert( desc.blendDesc.destinationColorBlend );
  psoDesc.BlendState.RenderTarget[ 0 ].BlendOp               = Convert( desc.blendDesc.colorBlendOperation );
  psoDesc.BlendState.RenderTarget[ 0 ].SrcBlendAlpha         = Convert( desc.blendDesc.sourceAlphaBlend );
  psoDesc.BlendState.RenderTarget[ 0 ].DestBlendAlpha        = Convert( desc.blendDesc.destinationAlphaBlend );
  psoDesc.BlendState.RenderTarget[ 0 ].BlendOpAlpha          = Convert( desc.blendDesc.alphaBlendOperation );
  psoDesc.BlendState.RenderTarget[ 0 ].LogicOp               = D3D12_LOGIC_OP_SET;
  psoDesc.BlendState.RenderTarget[ 0 ].RenderTargetWriteMask = desc.blendDesc.colorWrite ? D3D12_COLOR_WRITE_ENABLE_ALL : 0;

  psoDesc.RasterizerState.FillMode              = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode              = Convert( desc.rasterizerDesc.cullMode );
  psoDesc.RasterizerState.FrontCounterClockwise = desc.rasterizerDesc.cullFront == RasterizerDesc::CullFront::CounterClockwise;
  psoDesc.RasterizerState.DepthBias             = desc.rasterizerDesc.depthBias;
  psoDesc.RasterizerState.DepthBiasClamp        = 0;
  psoDesc.RasterizerState.SlopeScaledDepthBias  = 0;
  psoDesc.RasterizerState.DepthClipEnable       = TRUE;
  psoDesc.RasterizerState.MultisampleEnable     = FALSE;
  psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
  psoDesc.RasterizerState.ForcedSampleCount     = 0;
  psoDesc.RasterizerState.ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  psoDesc.DepthStencilState.DepthEnable                  = desc.depthStencilDesc.depthTest || desc.depthStencilDesc.depthWrite ? TRUE : FALSE;
  psoDesc.DepthStencilState.DepthWriteMask               = desc.depthStencilDesc.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
  psoDesc.DepthStencilState.DepthFunc                    = desc.depthStencilDesc.depthTest ? Convert( desc.depthStencilDesc.depthFunction ) : D3D12_COMPARISON_FUNC_ALWAYS;
  psoDesc.DepthStencilState.StencilEnable                = desc.depthStencilDesc.stencilEnable ? TRUE : FALSE;
  psoDesc.DepthStencilState.StencilReadMask              = desc.depthStencilDesc.stencilReadMask;
  psoDesc.DepthStencilState.StencilWriteMask             = desc.depthStencilDesc.stencilWriteMask;
  psoDesc.DepthStencilState.FrontFace.StencilFailOp      = Convert( desc.depthStencilDesc.stencilFrontFail );
  psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = Convert( desc.depthStencilDesc.stencilFrontDepthFail );
  psoDesc.DepthStencilState.FrontFace.StencilPassOp      = Convert( desc.depthStencilDesc.stencilFrontPass );
  psoDesc.DepthStencilState.FrontFace.StencilFunc        = Convert( desc.depthStencilDesc.stencilFrontFunction );
  psoDesc.DepthStencilState.BackFace.StencilFailOp       = Convert( desc.depthStencilDesc.stencilBackFail );
  psoDesc.DepthStencilState.BackFace.StencilDepthFailOp  = Convert( desc.depthStencilDesc.stencilBackDepthFail );
  psoDesc.DepthStencilState.BackFace.StencilPassOp       = Convert( desc.depthStencilDesc.stencilBackPass );
  psoDesc.DepthStencilState.BackFace.StencilFunc         = Convert( desc.depthStencilDesc.stencilBackFunction );

  psoDesc.DSVFormat = ConvertForDSV( Convert( desc.depthFormat ) );

  std::vector< D3D12_INPUT_ELEMENT_DESC > currentInputElements;
  currentInputElements.reserve( VertexDesc::MaxElements );
  for ( int elemIx = 0; elemIx < VertexDesc::MaxElements; ++elemIx )
  {
    auto& elem = desc.vertexDesc.elements[ elemIx ];
    if ( elem.dataType == VertexDesc::Element::DataType::None )
      break;

    currentInputElements.emplace_back();
    auto& d3dElem = currentInputElements.back();

    d3dElem.SemanticName         = elem.elementName;
    d3dElem.SemanticIndex        = elem.elementIndex;
    d3dElem.Format               = Convert( elem.dataType );
    d3dElem.InputSlot            = 0;
    d3dElem.AlignedByteOffset    = elem.dataOffset;
    d3dElem.InputSlotClass       = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    d3dElem.InstanceDataStepRate = 0;
  }

  psoDesc.InputLayout.pInputElementDescs = currentInputElements.data();
  psoDesc.InputLayout.NumElements        = UINT( currentInputElements.size() );

  psoDesc.IBStripCutValue = desc.indexSize == 16 ? D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF : D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;

  psoDesc.SampleDesc.Count = desc.samples;

  psoDesc.PrimitiveTopologyType = ConvertForRootSignature( desc.primitiveType );

  psoDesc.SampleMask = UINT_MAX;

  d3dDevice->CreateGraphicsPipelineState( &psoDesc, IID_PPV_ARGS( &d3dPipelineState ) );
}

D3DPipelineState::~D3DPipelineState()
{
}

ID3D12PipelineState* D3DPipelineState::GetD3DPipelineState()
{
  return d3dPipelineState;
}

ID3D12RootSignature* D3DPipelineState::GetD3DRootSignature()
{
  return d3dRootSignature;
}
