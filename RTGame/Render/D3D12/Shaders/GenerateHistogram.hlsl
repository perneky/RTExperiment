#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( UAV( u0 ) )" \

Texture2D< uint > LumaBuf : register( t0 );

RWByteAddressBuffer Histogram : register( u0 );

groupshared uint tileHistogram[ 256 ];

[RootSignature( _RootSignature )]
[numthreads( GenerateHistogramKernelWidth, GenerateHistogramKernelHeight, 1 )]
void main( uint GI : SV_GroupIndex, uint3 DTid : SV_DispatchThreadID )
{
  tileHistogram[ GI ] = 0;

  GroupMemoryBarrierWithGroupSync();

  for ( uint top = 0; top < 384; top += 16 )
  {
    uint quantizedLogLuma = LumaBuf[ DTid.xy + uint2( 0, top ) ];
    InterlockedAdd( tileHistogram[ quantizedLogLuma ], 1 );
  }

  GroupMemoryBarrierWithGroupSync();

  Histogram.InterlockedAdd( GI * 4, tileHistogram[ GI ] );
}
