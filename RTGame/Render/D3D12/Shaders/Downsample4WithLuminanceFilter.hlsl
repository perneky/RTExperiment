#include "../../ShaderValues.h"
#include "Utils.hlsli"
#define _RootSignature "RootFlags( 0 )," \
                       "DescriptorTable( SRV( t0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )" \

Texture2D<float4>   source      : register( t0 );
RWTexture2D<float4> destination : register( u0 );

[RootSignature( _RootSignature )]
[numthreads( DownsamplingKernelWidth, DownsamplingKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  uint2 uv = dispatchThreadID.xy * 4;

  float4 samples[ 16 ] =
  {
    source[ uv + uint2( 0, 0 ) ],
    source[ uv + uint2( 1, 0 ) ],
    source[ uv + uint2( 2, 0 ) ],
    source[ uv + uint2( 3, 0 ) ],
    source[ uv + uint2( 0, 1 ) ],
    source[ uv + uint2( 1, 1 ) ],
    source[ uv + uint2( 2, 1 ) ],
    source[ uv + uint2( 3, 1 ) ],
    source[ uv + uint2( 0, 2 ) ],
    source[ uv + uint2( 1, 2 ) ],
    source[ uv + uint2( 2, 2 ) ],
    source[ uv + uint2( 3, 2 ) ],
    source[ uv + uint2( 0, 3 ) ],
    source[ uv + uint2( 1, 3 ) ],
    source[ uv + uint2( 2, 3 ) ],
    source[ uv + uint2( 3, 3 ) ],
  };

  float lightness[ 16 ];

  [unroll]
  for ( uint ix = 0; ix < 16; ++ix )
    lightness[ ix ] = Lightness( samples[ ix ].rgb );

  float midLight = 10000;

  [unroll]
  for ( ix = 0; ix < 16; ++ix )
    midLight = min( lightness[ ix ], midLight );

  float3 result = 0;
  float  allW   = 0;
  float  alpha  = 0;

  [unroll]
  for ( ix = 0; ix < 16; ++ix )
  {
    float d = abs( lightness[ ix ] - midLight );
    float w = saturate( 1.0 - ( d * d ) );

    alpha  += samples[ ix ].a;
    allW   += w;
    result += samples[ ix ].rgb * w;
  }

  destination[ dispatchThreadID.xy ] = float4( result / allW, alpha / 16 );
}