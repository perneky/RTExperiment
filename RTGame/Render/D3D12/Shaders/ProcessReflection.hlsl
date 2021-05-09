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

float3 CalcWorldPosition( uint2 uv )
{
  float  depth     = LinearizeDepth( depthTexture[ uv ].r, invProj );
  float3 topDir    = lerp( cornerWorldDirs[ 0 ].xyz, cornerWorldDirs[ 1 ].xyz, ( uv.x + 0.5 ) * invTexSize.x );
  float3 bottomDir = lerp( cornerWorldDirs[ 2 ].xyz, cornerWorldDirs[ 3 ].xyz, ( uv.x + 0.5 ) * invTexSize.x );
  float3 worldDir  = normalize( lerp( topDir, bottomDir, ( uv.y + 0.5 ) * invTexSize.y ) );

  return cameraPosition.xyz + worldDir * depth;
}

float3 CalcWorldPositionAndNormal( uint2 uv, out float3 normal )
{
  float3 pos00 = CalcWorldPosition( uv );
  float3 pos10 = CalcWorldPosition( uv + uint2( 1, 0 ) );
  float3 pos01 = CalcWorldPosition( uv + uint2( 0, 1 ) );

  normal = normalize( cross( pos10 - pos00, pos01 - pos00 ) );

  return pos00;
}

float3 SampleReflection( uint2 localTc, float3 originPos, float3 originNormal, inout float weightSum )
{
  float3 normal;
  float3 posTap  = CalcWorldPositionAndNormal( localTc, normal );
  float  weightD = saturate( 2 - length( posTap - originPos ) ) * 0.5;
  float  weightA = saturate( dot( originNormal, normal ) );
  float  weight  = weightD * weightA * weightA * weightA;

  weightSum += weight;

  [branch]
  if ( weight > 0 )
    return sourceTexture[ localTc ].rgb * weight;
  else
    return 0;
}

[RootSignature( _RootSignature )]
[numthreads( LightCombinerKernelWidth, LightCombinerKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  [branch]
  if ( any( dispatchThreadID.xy >= texSize ) )
    return;

  float3 originNormal;
  float3 originPos = CalcWorldPositionAndNormal( dispatchThreadID.xy, originNormal );
  float4 blurRough = sourceTexture[ dispatchThreadID.xy ];
  float  roughness = blurRough.a;
  float3 blurred   = 0;
  float  weightSum = 0;

  [branch]
  if ( dir == 0 )
    for ( uint tap = 0; tap < texSize.y; tap++ )
      blurred += SampleReflection( uint2( dispatchThreadID.x, tap ), originPos, originNormal, weightSum );
  else
    for ( uint tap = 0; tap < texSize.x; tap++ )
      blurred += SampleReflection( uint2( tap, dispatchThreadID.y ), originPos, originNormal, weightSum );

  if ( weightSum == 0 )
  {
    blurred   = blurRough.rgb;
    weightSum = 1;
  }

  destinationTexture[ dispatchThreadID.xy ] = float4( blurred / weightSum, roughness );
}