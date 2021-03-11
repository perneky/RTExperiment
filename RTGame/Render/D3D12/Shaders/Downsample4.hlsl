#include "../../ShaderValues.h"
#define _RootSignature "RootFlags( 0 )," \
                       "DescriptorTable( SRV( t0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )" \

Texture2D<float4>   source      : register( t0 );
RWTexture2D<float4> destination : register( u0 );

float4 DS2( int2 x )
{
  float4 src1 = source[ x + uint2( 0, 0 ) ];
  float4 src2 = source[ x + uint2( 0, 1 ) ];
  float4 src3 = source[ x + uint2( 1, 0 ) ];
  float4 src4 = source[ x + uint2( 1, 1 ) ];
  return ( src1 + src2 + src3 + src4 ) / 4;
}

float4 DS4( int2 x )
{
  float4 src1 = DS2( x + uint2( 0, 0 ) );
  float4 src2 = DS2( x + uint2( 0, 2 ) );
  float4 src3 = DS2( x + uint2( 2, 0 ) );
  float4 src4 = DS2( x + uint2( 2, 2 ) );
  return ( src1 + src2 + src3 + src4 ) / 4;
}

[RootSignature( _RootSignature )]
[numthreads( DownsamplingKernelWidth, DownsamplingKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  destination[ dispatchThreadID.xy ] = DS4( dispatchThreadID.xy * 4 );
}