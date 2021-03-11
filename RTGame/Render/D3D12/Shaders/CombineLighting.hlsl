#include "../../ShaderValues.h"
#include "RootSignatures/ShaderStructures.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 3 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( SRV( t1 ) )," \
                       "DescriptorTable( SRV( t2 ) )," \
                       "DescriptorTable( SRV( t3 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \

SamplerState linearSampler : register( s0 );

cbuffer Params : register( b0 )
{
  float2           invTexSize;
  FrameDebugModeCB frameDebugMode;
}

Texture2D<half4>   directLightingTexture    : register( t0 );
Texture2D<half4>   specularIBLTexture       : register( t1 );
Texture2D<half4>   hiReflectionTexture      : register( t2 );
Texture2D<half4>   loReflectionTexture      : register( t3 );
RWTexture2D<half4> combinedTexture          : register( u0 );

[RootSignature( _RootSignature )]
[numthreads( LightCombinerKernelWidth, LightCombinerKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  float2 tc                  = ( dispatchThreadID.xy + 0.5 ) * invTexSize;
  float3 directLighting      = directLightingTexture[ dispatchThreadID.xy ].rgb;
  float3 specularIBL         = specularIBLTexture[ dispatchThreadID.xy ].rgb;
  float3 roughness           = hiReflectionTexture[ dispatchThreadID.xy ].a;
  float3 hiSurfaceReflection = hiReflectionTexture[ dispatchThreadID.xy ].rgb;
  float3 loSurfaceReflection = loReflectionTexture.SampleLevel( linearSampler, tc, 0 ).rgb;
  float3 surfaceReflection   = lerp( hiSurfaceReflection, loSurfaceReflection, saturate( roughness * 10 ) );

  float3 combinedLighting;

  switch ( frameDebugMode )
  {
  case FrameDebugModeCB::None:
    combinedLighting = directLighting + surfaceReflection * specularIBL;
    break;
  case FrameDebugModeCB::ShowProcessedReflection:
    combinedLighting = surfaceReflection;
    break;
  default:
    combinedLighting = directLighting;
    break;
  }

  combinedTexture[ dispatchThreadID.xy ] = float4( combinedLighting, 1 );
}