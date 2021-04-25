#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 3 )," \
                       "CBV( b1 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "DescriptorTable( UAV( u1 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \

cbuffer Params : register( b0 )
{
  float2 inverseOutputSize;
  float  bloomThreshold;
}

cbuffer Exposure : register( b1 )
{
  ExposureBufferCB exposure;
}

Texture2D< float3 >   SourceTex   : register( t0 );
RWTexture2D< float3 > BloomResult : register( u0 );
RWTexture2D< uint >   LumaResult  : register( u1 );

SamplerState BiLinearClamp : register( s0 );

[ RootSignature( _RootSignature ) ]
[ numthreads( ExtractBloomKernelWidth, ExtractBloomKernelHeight, 1 ) ]
void main( uint3 DTid : SV_DispatchThreadID )
{
  float2 uv = ( DTid.xy + 0.5 ) * inverseOutputSize;
  float2 offset = inverseOutputSize * 0.25;

  float3 color1 = SourceTex.SampleLevel( BiLinearClamp, uv + float2( -offset.x, -offset.y ), 0 );
  float3 color2 = SourceTex.SampleLevel( BiLinearClamp, uv + float2(  offset.x, -offset.y ), 0 );
  float3 color3 = SourceTex.SampleLevel( BiLinearClamp, uv + float2( -offset.x,  offset.y ), 0 );
  float3 color4 = SourceTex.SampleLevel( BiLinearClamp, uv + float2(  offset.x,  offset.y ), 0 );

  float luma1 = Lightness( color1 );
  float luma2 = Lightness( color2 );
  float luma3 = Lightness( color3 );
  float luma4 = Lightness( color4 );

  float smallEpsilon    = 0.0001;
  float scaledThreshold = bloomThreshold / exposure.exposure;

  color1 *= max( smallEpsilon, luma1 - scaledThreshold ) / ( luma1 + smallEpsilon );
  color2 *= max( smallEpsilon, luma2 - scaledThreshold ) / ( luma2 + smallEpsilon );
  color3 *= max( smallEpsilon, luma3 - scaledThreshold ) / ( luma3 + smallEpsilon );
  color4 *= max( smallEpsilon, luma4 - scaledThreshold ) / ( luma4 + smallEpsilon );

  float shimmerFilterInverseStrength = 1.0f;
  float weight1 = 1.0f / ( luma1 + shimmerFilterInverseStrength );
  float weight2 = 1.0f / ( luma2 + shimmerFilterInverseStrength );
  float weight3 = 1.0f / ( luma3 + shimmerFilterInverseStrength );
  float weight4 = 1.0f / ( luma4 + shimmerFilterInverseStrength );
  float weightSum = weight1 + weight2 + weight3 + weight4;

  BloomResult[ DTid.xy ] = ( color1 * weight1 + color2 * weight2 + color3 * weight3 + color4 * weight4 ) / weightSum;

  float luma = ( luma1 + luma2 + luma3 + luma4 ) * 0.25;

  if ( luma == 0.0 )
    LumaResult[ DTid.xy ] = 0;
  else
    LumaResult[ DTid.xy ] = saturate( ( log2( luma ) - exposure.minLog ) * exposure.invLogRange ) * 254.0 + 1.0;
}
