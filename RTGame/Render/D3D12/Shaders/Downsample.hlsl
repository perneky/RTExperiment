#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "DescriptorTable( SRV( t0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )," \

Texture2D<float4>   source      : register( t0 );
RWTexture2D<float4> destination : register( u0 );

SamplerState linearSampler : register( s0 );

[RootSignature( _RootSignature )]
[numthreads( DownsamplingKernelWidth, DownsamplingKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  float2 texSize;
  source.GetDimensions( texSize.x, texSize.y );

  float2 tc = ( dispatchThreadID.xy * 2 + 0.5 ) / texSize;

  destination[ dispatchThreadID.xy ] = source.SampleLevel( linearSampler, tc, 0 );
}