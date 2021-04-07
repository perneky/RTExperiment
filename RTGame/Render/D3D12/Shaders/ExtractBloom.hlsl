#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 3 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( SRV( t1 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \

cbuffer cb0 : register( b0 )
{
  float2 g_inverseOutputSize;
  float g_bloomThreshold;
}

Texture2D<float3> SourceTex : register( t0 );
Texture2D<float> Exposure : register( t1 );
RWTexture2D<float3> BloomResult : register( u0 );

SamplerState BiLinearClamp : register( s0 );

[ RootSignature( _RootSignature ) ]
[ numthreads( ExtractBloomKernelWidth, ExtractBloomKernelHeight, 1 ) ]
void main( uint3 DTid : SV_DispatchThreadID )
{
  // We need the scale factor and the size of one pixel so that our four samples are right in the middle
  // of the quadrant they are covering.
  float2 uv = ( DTid.xy + 0.5 ) * g_inverseOutputSize;
  float2 offset = g_inverseOutputSize * 0.25;

  // Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
  float3 color1 = SourceTex.SampleLevel( BiLinearClamp, uv + float2( -offset.x, -offset.y ), 0 );
  float3 color2 = SourceTex.SampleLevel( BiLinearClamp, uv + float2( offset.x, -offset.y ), 0 );
  float3 color3 = SourceTex.SampleLevel( BiLinearClamp, uv + float2( -offset.x, offset.y ), 0 );
  float3 color4 = SourceTex.SampleLevel( BiLinearClamp, uv + float2( offset.x, offset.y ), 0 );

  float luma1 = Lightness( color1 );
  float luma2 = Lightness( color2 );
  float luma3 = Lightness( color3 );
  float luma4 = Lightness( color4 );

  const float kSmallEpsilon = 0.0001;

  float ScaledThreshold = g_bloomThreshold / Exposure[ int2( 0, 0 ) ];

  // We perform a brightness filter pass, where lone bright pixels will contribute less.
  color1 *= max( kSmallEpsilon, luma1 - ScaledThreshold ) / ( luma1 + kSmallEpsilon );
  color2 *= max( kSmallEpsilon, luma2 - ScaledThreshold ) / ( luma2 + kSmallEpsilon );
  color3 *= max( kSmallEpsilon, luma3 - ScaledThreshold ) / ( luma3 + kSmallEpsilon );
  color4 *= max( kSmallEpsilon, luma4 - ScaledThreshold ) / ( luma4 + kSmallEpsilon );

  // The shimmer filter helps remove stray bright pixels from the bloom buffer by inversely weighting
  // them by their luminance.  The overall effect is to shrink bright pixel regions around the border.
  // Lone pixels are likely to dissolve completely.  This effect can be tuned by adjusting the shimmer
  // filter inverse strength.  The bigger it is, the less a pixel's luminance will matter.
  const float kShimmerFilterInverseStrength = 1.0f;
  float weight1 = 1.0f / ( luma1 + kShimmerFilterInverseStrength );
  float weight2 = 1.0f / ( luma2 + kShimmerFilterInverseStrength );
  float weight3 = 1.0f / ( luma3 + kShimmerFilterInverseStrength );
  float weight4 = 1.0f / ( luma4 + kShimmerFilterInverseStrength );
  float weightSum = weight1 + weight2 + weight3 + weight4;

  BloomResult[ DTid.xy ] = ( color1 * weight1 + color2 * weight2 + color3 * weight3 + color4 * weight4 ) / weightSum;
}
