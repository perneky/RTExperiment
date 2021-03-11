#include "../../ShaderValues.h"
#include "RootSignatures/ShaderStructures.hlsli"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = " ReflectionProcessDataSizeStr " )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( SRV( t1 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )," \
                       "StaticSampler( s1," \
                       "               filter = FILTER_MIN_MAG_MIP_POINT," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \

cbuffer Params : register( b0 )
{
  matrix invProj;
  int2   texSize;
  float2 invTexSize;
  float2 blurKernel[ ReflectionProcessTaps ];
}

SamplerState linearSampler : register( s0 );
SamplerState pointSampler  : register( s1 );

Texture2D<half4>   depthTexture       : register( t0 );
Texture2D<half4>   sourceTexture      : register( t1 );
RWTexture2D<half4> destinationTexture : register( u0 );

[RootSignature( _RootSignature )]
[numthreads( LightCombinerKernelWidth, LightCombinerKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  [branch]
  if ( any( dispatchThreadID.xy >= texSize ) )
    return;

  float2 tc          = ( dispatchThreadID.xy + 0.5 ) * invTexSize;
  float  depthOrigin = LinearizeDepth( depthTexture.SampleLevel( linearSampler, tc, 0 ).r, invProj );
  float4 blurRough   = sourceTexture.SampleLevel( pointSampler, tc, 0 );
  float  roughness   = blurRough.a;
  float3 blurred     = 0;
  float  samples     = 1;

  for ( int tap = 0; tap < ReflectionProcessTaps; tap++ )
  {
    float2 localTc = tc + blurKernel[ tap ] * roughness;

    float depthTap = LinearizeDepth( depthTexture.SampleLevel( linearSampler, localTc, 0 ).r, invProj );
    [branch]
    if ( abs( depthTap - depthOrigin ) < 2 )
    {
      blurred += sourceTexture.SampleLevel( linearSampler, localTc, 0 ).rgb;
      samples += 1;
    }
  }

  if ( samples == 0 )
    blurred = blurRough.rgb;

  destinationTexture[ dispatchThreadID.xy ] = float4( blurred / samples, roughness );
}