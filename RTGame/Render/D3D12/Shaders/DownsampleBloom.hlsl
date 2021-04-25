#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 2 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "DescriptorTable( UAV( u1 ) )," \
                       "DescriptorTable( UAV( u2 ) )," \
                       "DescriptorTable( UAV( u3 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \

Texture2D< float3 > BloomBuf : register( t0 );

RWTexture2D< float3 > Result1 : register( u0 );
RWTexture2D< float3 > Result2 : register( u1 );
RWTexture2D< float3 > Result3 : register( u2 );
RWTexture2D< float3 > Result4 : register( u3 );

SamplerState BiLinearClamp : register( s0 );

cbuffer cb0 : register( b0 )
{
  float2 inverseDimensions;
}

groupshared float3 tile[64];

[RootSignature( _RootSignature )]
[numthreads( DownsampleBloomKernelWidth, DownsampleBloomKernelHeight, 1 )]
void main( uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID )
{
  uint parity = DTid.x | DTid.y;

  float2 centerUV = ( float2( DTid.xy ) * 2.0f + 1.0f ) * inverseDimensions;
  float3 avgPixel = BloomBuf.SampleLevel( BiLinearClamp, centerUV, 0.0f );
  
  tile[ GI ] = avgPixel;
  Result1[ DTid.xy ] = avgPixel;

  GroupMemoryBarrierWithGroupSync();

  if ( ( parity & 1 ) == 0 )
  {
    avgPixel = 0.25f * ( avgPixel + tile[ GI + 1 ] + tile[ GI + 8 ] + tile[ GI + 9 ] );
    tile[ GI ] = avgPixel;
    Result2[ DTid.xy >> 1 ] = avgPixel;
  }

  GroupMemoryBarrierWithGroupSync();

  if ( ( parity & 3 ) == 0 )
  {
    avgPixel = 0.25f * ( avgPixel + tile[ GI + 2 ] + tile[ GI + 16 ] + tile[ GI + 18 ] );
    tile[ GI ] = avgPixel;
    Result3[ DTid.xy >> 2 ] = avgPixel;
  }

  GroupMemoryBarrierWithGroupSync();

  if ( ( parity & 7 ) == 0 )
  {
      avgPixel = 0.25f * ( avgPixel + tile[ GI + 4 ] + tile[ GI + 32 ] + tile[ GI + 36 ] );
      Result4[ DTid.xy >> 3 ] = avgPixel;
  }
}
