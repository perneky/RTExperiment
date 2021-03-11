#include "../../ShaderValues.h"
#define _RootSignature "RootFlags( 0 )," \
                       "DescriptorTable( CBV( b0 ) )," \
                       "DescriptorTable( SRV( t0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \

cbuffer Parameters : register( b0 )
{
  float4 invTextureSize;
  float4 sampleOffsets[ 17 ];
  float4 sampleWeights[ 17 ];
};

Texture2D<float4>   source      : register( t0 );
RWTexture2D<float4> destination : register( u0 );

SamplerState blurSampler : register( s0 );

[RootSignature( _RootSignature )]
[numthreads( BlurKernelWidth, BlurKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  float4 color    = 0;
  float2 texcoord = invTextureSize.xy * ( float2( dispatchThreadID.xy ) + 0.5 );

  [unroll]
  for ( int i = 0; i < 17; i++ )
    color += source.SampleLevel( blurSampler, texcoord + sampleOffsets[ i ].xy, 0 ) * sampleWeights[ i ];

  destination[ dispatchThreadID.xy ] = color;
}