#include "../../ShaderValues.h"
#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 3 )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( UAV( u1, flags = DATA_VOLATILE ) )" \

cbuffer Params : register( b0 )
{
  float manualExposure;
  float timeElapsed;
  float speed;
};

RWTexture2D<float4> averageExposure : register( u0 );
RWTexture2D<float>  exposure        : register( u1 );

static const float3 gsHelper = float3( 0.3, 0.59, 0.11 );

[RootSignature( _RootSignature )]
[numthreads( 1, 1, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  float c = exposure[ int2( 0, 0 ) ];
  float l = dot( averageExposure[ dispatchThreadID.xy ].rgb, gsHelper );
  float d = 1.0 - l;
  float e = lerp( c, c + d, timeElapsed * speed );

  exposure[ int2( 0, 0 ) ] = manualExposure > 0 ? manualExposure : e;
}