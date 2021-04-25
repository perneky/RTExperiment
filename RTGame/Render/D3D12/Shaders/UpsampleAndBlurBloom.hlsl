#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 3 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( SRV( t1 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_BORDER," \
                       "               addressV = TEXTURE_ADDRESS_BORDER," \
                       "               addressW = TEXTURE_ADDRESS_BORDER," \
                       "               borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK )" \

cbuffer cb0 : register( b0 )
{
  float2 inverseDimensions;
  float  upsampleBlendFactor;
}

Texture2D< float3 > HigherResBuf : register( t0 );
Texture2D< float3 > LowerResBuf  : register( t1 );

RWTexture2D< float3 > Result : register( u0 );

SamplerState LinearBorder : register( s0 );

static const float weights5[ 3 ] = { 6.0f / 16.0f, 4.0f / 16.0f, 1.0f / 16.0f };
static const float weights7[ 4 ] = { 20.0f / 64.0f, 15.0f / 64.0f, 6.0f / 64.0f, 1.0f / 64.0f };
static const float weights9[ 5 ] = { 70.0f / 256.0f, 56.0f / 256.0f, 28.0f / 256.0f, 8.0f / 256.0f, 1.0f / 256.0f };

float3 Blur5( float3 a, float3 b, float3 c, float3 d, float3 e, float3 f, float3 g, float3 h, float3 i )
{
  return weights5[ 0 ] * e + weights5[ 1 ] * ( d + f ) + weights5[ 2 ] * ( c + g );
}

float3 Blur7( float3 a, float3 b, float3 c, float3 d, float3 e, float3 f, float3 g, float3 h, float3 i )
{
  return weights7[ 0 ] * e + weights7[ 1 ] * ( d + f ) + weights7[ 2 ] * ( c + g ) + weights7[ 3 ] * ( b + h );
}

float3 Blur9( float3 a, float3 b, float3 c, float3 d, float3 e, float3 f, float3 g, float3 h, float3 i )
{
  return weights9[ 0 ] * e + weights9[ 1 ] * ( d + f ) + weights9[ 2 ] * ( c + g ) + weights9[ 3 ] * ( b + h ) + weights9[ 4 ] * ( a + i );
}

#define BlurPixels Blur9

groupshared uint CacheR[ 128 ];
groupshared uint CacheG[ 128 ];
groupshared uint CacheB[ 128 ];

void Store2Pixels( uint index, float3 pixel1, float3 pixel2 )
{
  CacheR[ index ] = f32tof16( pixel1.r ) | f32tof16( pixel2.r ) << 16;
  CacheG[ index ] = f32tof16( pixel1.g ) | f32tof16( pixel2.g ) << 16;
  CacheB[ index ] = f32tof16( pixel1.b ) | f32tof16( pixel2.b ) << 16;
}

void Load2Pixels( uint index, out float3 pixel1, out float3 pixel2 )
{
  uint3 RGB = uint3( CacheR[ index ], CacheG[ index ], CacheB[ index ] );
  pixel1 = f16tof32( RGB );
  pixel2 = f16tof32( RGB >> 16 );
}

void Store1Pixel( uint index, float3 pixel )
{
  CacheR[ index ] = asuint( pixel.r );
  CacheG[ index ] = asuint( pixel.g );
  CacheB[ index ] = asuint( pixel.b );
}

void Load1Pixel( uint index, out float3 pixel )
{
  pixel = asfloat( uint3( CacheR[ index ], CacheG[ index ], CacheB[ index ] ) );
}

void BlurHorizontally( uint outIndex, uint leftMostIndex )
{
  float3 s0, s1, s2, s3, s4, s5, s6, s7, s8, s9;
  Load2Pixels( leftMostIndex + 0, s0, s1 );
  Load2Pixels( leftMostIndex + 1, s2, s3 );
  Load2Pixels( leftMostIndex + 2, s4, s5 );
  Load2Pixels( leftMostIndex + 3, s6, s7 );
  Load2Pixels( leftMostIndex + 4, s8, s9 );
  
  Store1Pixel( outIndex,     BlurPixels( s0, s1, s2, s3, s4, s5, s6, s7, s8 ) );
  Store1Pixel( outIndex + 1, BlurPixels( s1, s2, s3, s4, s5, s6, s7, s8, s9 ) );
}

void BlurVertically( uint2 pixelCoord, uint topMostIndex )
{
  float3 s0, s1, s2, s3, s4, s5, s6, s7, s8;
  Load1Pixel( topMostIndex,      s0 );
  Load1Pixel( topMostIndex +  8, s1 );
  Load1Pixel( topMostIndex + 16, s2 );
  Load1Pixel( topMostIndex + 24, s3 );
  Load1Pixel( topMostIndex + 32, s4 );
  Load1Pixel( topMostIndex + 40, s5 );
  Load1Pixel( topMostIndex + 48, s6 );
  Load1Pixel( topMostIndex + 56, s7 );
  Load1Pixel( topMostIndex + 64, s8 );

  Result[pixelCoord] = BlurPixels( s0, s1, s2, s3, s4, s5, s6, s7, s8 );
}

[RootSignature( _RootSignature )]
[numthreads( UpsampleBlurBloomKernelWidth, UpsampleBlurBloomKernelHeight, 1 )]
void main( uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID )
{
    int2 GroupUL  = ( Gid.xy  << 3 ) - 4;
    int2 ThreadUL = ( GTid.xy << 1 ) + GroupUL;

    float2 uvUL    = ( float2( ThreadUL ) + 0.5 ) * inverseDimensions;
    float2 uvLR    = uvUL + inverseDimensions;
    float2 uvUR    = float2( uvLR.x, uvUL.y );
    float2 uvLL    = float2( uvUL.x, uvLR.y );
    int    destIdx = GTid.x + ( GTid.y << 4 );

    float3 pixel1a = lerp( HigherResBuf[ ThreadUL + uint2( 0, 0 ) ], LowerResBuf.SampleLevel( LinearBorder, uvUL, 0.0f ), upsampleBlendFactor );
    float3 pixel1b = lerp( HigherResBuf[ ThreadUL + uint2( 1, 0 ) ], LowerResBuf.SampleLevel( LinearBorder, uvUR, 0.0f ), upsampleBlendFactor );
    Store2Pixels( destIdx + 0, pixel1a, pixel1b );

    float3 pixel2a = lerp( HigherResBuf[ ThreadUL + uint2( 0, 1 ) ], LowerResBuf.SampleLevel( LinearBorder, uvLL, 0.0f ), upsampleBlendFactor );
    float3 pixel2b = lerp( HigherResBuf[ ThreadUL + uint2( 1, 1 ) ], LowerResBuf.SampleLevel( LinearBorder, uvLR, 0.0f ), upsampleBlendFactor );
    Store2Pixels( destIdx + 8, pixel2a, pixel2b );

    GroupMemoryBarrierWithGroupSync();

    uint row = GTid.y << 4;
    BlurHorizontally( row + ( GTid.x << 1 ), row + GTid.x + ( GTid.x & 4 ) );

    GroupMemoryBarrierWithGroupSync();

    BlurVertically( DTid.xy, ( GTid.y << 3 ) + GTid.x );
}
