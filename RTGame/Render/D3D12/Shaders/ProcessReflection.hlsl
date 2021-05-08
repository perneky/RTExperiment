#include "../../ShaderValues.h"
#include "RootSignatures/ShaderStructures.hlsli"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 42 )," \
                       "CBV( b1 )," \
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
  float4 cornerWorldDirs[ 4 ];
  float4 cameraPosition;
  int2   texSize;
  float2 invTexSize;
  int    dir;
}

ConstantBuffer< HaltonSequenceCB > haltonSequence : register( b1 );

SamplerState linearSampler : register( s0 );
SamplerState pointSampler  : register( s1 );

Texture2D<half4>   depthTexture       : register( t0 );
Texture2D<half4>   sourceTexture      : register( t1 );
RWTexture2D<half4> destinationTexture : register( u0 );

float3 CalcWorldPosition( float2 uv, float depth )
{
  float3 topDir    = lerp( cornerWorldDirs[ 0 ].xyz, cornerWorldDirs[ 1 ].xyz, uv.x );
  float3 bottomDir = lerp( cornerWorldDirs[ 2 ].xyz, cornerWorldDirs[ 3 ].xyz, uv.x );
  float3 worldDir  = normalize( lerp( topDir, bottomDir, uv.y ) );

  return cameraPosition.xyz + worldDir * depth;
}

float3 CalcWorldPosition( float2 uv )
{
  float depth = LinearizeDepth( depthTexture.SampleLevel( linearSampler, uv, 0 ).r, invProj );
  return CalcWorldPosition( uv, depth );
}

[RootSignature( _RootSignature )]
[numthreads( LightCombinerKernelWidth, LightCombinerKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  [branch]
  if ( any( dispatchThreadID.xy >= texSize ) )
    return;

  float2 tc           = ( dispatchThreadID.xy + 0.5 ) * invTexSize;
  float  originDepthT = depthTexture.SampleLevel( linearSampler, tc, 0 ).r;
  float  originDepth  = LinearizeDepth( originDepthT, invProj );
  float3 originPos    = CalcWorldPosition( tc, originDepth );
  float4 blurRough    = sourceTexture.SampleLevel( pointSampler, tc, 0 );
  float  roughness    = blurRough.a;
  float3 blurred      = 0;
  float  samples      = 0;

  [branch]
  if ( dir == 0 )
  {
    for ( int tap = 0; tap < texSize.y; tap++ )
    {
      float2 localTc = float2( ( dispatchThreadID.x + 0.5 ), tap + 0.5 ) * invTexSize;
      float3 posTap  = CalcWorldPosition( localTc );

      [branch]
      if ( length( posTap - originPos ) < 2 )
      {
        blurred += sourceTexture.SampleLevel( linearSampler, localTc, 0 ).rgb;
        samples += 1;
      }
    }
  }
  else
  {
    for ( int tap = 0; tap < texSize.x; tap++ )
    {
      float2 localTc = float2( tap + 0.5, ( dispatchThreadID.y + 0.5 ) ) * invTexSize;
      float3 posTap  = CalcWorldPosition( localTc );

      [branch]
      if ( length( posTap - originPos ) < 2 )
      {
        blurred += sourceTexture.SampleLevel( linearSampler, localTc, 0 ).rgb;
        samples += 1;
      }
    }
  }

  if ( samples == 0 )
  {
    blurred = blurRough.rgb;
    samples = 1;
  }

  destinationTexture[ dispatchThreadID.xy ] = float4( blurred / samples, roughness );
}