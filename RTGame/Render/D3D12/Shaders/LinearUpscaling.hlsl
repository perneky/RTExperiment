#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 2 )," \
                       "DescriptorTable( SRV( t0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )," \

cbuffer Params : register( b0 )
{
  float2 invDstTexSize;
};

Texture2D<float4>   sourceColor      : register( t0 );
RWTexture2D<float4> destinationColor : register( u0 );

SamplerState linearSampler : register( s0 );

[RootSignature( _RootSignature )]
[numthreads( LinearUSKernelWidth, LinearUSKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  float2 tc = ( dispatchThreadID.xy + 0.5 ) * invDstTexSize;

  destinationColor[ dispatchThreadID.xy ] = sourceColor.SampleLevel( linearSampler, tc, 0 );
}