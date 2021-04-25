#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 5 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "DescriptorTable( UAV( u1 ) )" \

cbuffer cb0 : register( b0 )
{
    float TargetLuminance;
    float AdaptationRate;
    float MinExposure;
    float MaxExposure;
    uint  PixelCount; 
}

ByteAddressBuffer                      Histogram    : register( t0 );
RWStructuredBuffer< ExposureBufferCB > Exposure     : register( u0 );
RWTexture2D< float >                   ExposureOnly : register( u1 );

groupshared float accum[ 256 ];

[RootSignature( _RootSignature )]
[numthreads( 256, 1, 1 )]
void main( uint GI : SV_GroupIndex )
{
  float weightedSum = (float)GI * (float)Histogram.Load( GI * 4 );

  [unroll]
  for ( uint i = 1; i < 256; i *= 2 )
  {
    accum[ GI ] = weightedSum;
    GroupMemoryBarrierWithGroupSync();
    weightedSum += accum[ ( GI + i ) % 256 ];
    GroupMemoryBarrierWithGroupSync();
  }

  float MinLog          = Exposure[ 0 ].minLog;
  float MaxLog          = Exposure[ 0 ].maxLog;
  float LogRange        = Exposure[ 0 ].logRange;
  float RcpLogRange     = Exposure[ 0 ].invLogRange;
  float weightedHistAvg = weightedSum / ( max( 1, PixelCount - Histogram.Load( 0 ) ) ) - 1.0;
  float logAvgLuminance = exp2( weightedHistAvg / 254.0 * LogRange + MinLog );
  float targetExposure  = TargetLuminance / logAvgLuminance;
  float exposure        = Exposure[ 0 ].exposure;

  exposure = lerp( exposure, targetExposure, AdaptationRate );
  exposure = clamp( exposure, MinExposure, MaxExposure );

  [branch]
  if ( GI == 0 )
  {
    ExposureOnly[ uint2( 0, 0 ) ] = exposure;

    Exposure[ 0 ].exposure        = exposure;
    Exposure[ 0 ].invExposure     = 1.0 / exposure;
    Exposure[ 0 ].targetExposure  = exposure;
    Exposure[ 0 ].weightedHistAvg = weightedHistAvg;

    float biasToCenter = ( floor( weightedHistAvg ) - 128.0 ) / 255.0;
    if ( abs( biasToCenter ) > 0.1 )
    {
        MinLog += biasToCenter * RcpLogRange;
        MaxLog += biasToCenter * RcpLogRange;
    }

    Exposure[ 0 ].minLog      = MinLog;
    Exposure[ 0 ].maxLog      = MaxLog;
    Exposure[ 0 ].logRange    = LogRange;
    Exposure[ 0 ].invLogRange = 1.0 / LogRange;
  }
}
