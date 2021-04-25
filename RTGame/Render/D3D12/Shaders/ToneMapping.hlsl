#include "../../ShaderValues.h"
#include "/RootSignatures/ShaderStructures.hlsli"
#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 3 )," \
                       "CBV( b1, flags = DATA_VOLATILE )," \
                       "DescriptorTable( SRV( t0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( SRV( t1, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )"

cbuffer Params : register( b0 )
{
  float2 invTexSize;
  float  bloomStrength;
};

cbuffer Exposure : register( b1 )
{
  ExposureBufferCB exposure;
}

Texture2D<float4>       source      : register( t0 );
Texture2D<float4>       bloom       : register( t1 );
RWTexture2D<float4>     destination : register( u0 );

SamplerState linearSampler : register( s0 );

float3 TM_Reinhard( float3 hdr, float k = 1.0 )
{
  return hdr / ( hdr + k );
}

float3 TM_Stanard( float3 hdr )
{
  return TM_Reinhard( hdr * sqrt( hdr ), sqrt( 4.0 / 27.0 ) );
}

[RootSignature( _RootSignature )]
[numthreads( ToneMappingKernelWidth, ToneMappingKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  float3 bloomSample = bloom.SampleLevel( linearSampler, ( dispatchThreadID.xy + 0.5 ) * invTexSize, 0 ).rgb * bloomStrength;
  float3 hdrColor    = ( source[ dispatchThreadID.xy ].rgb + bloomSample ) * exposure.exposure;
  float3 sdrColor    = TM_Stanard( hdrColor );
  destination[ dispatchThreadID.xy ] = float4( sdrColor, 1.0 );
}