#include "../../ShaderValues.h"
#define _RootSignature "RootFlags( 0 )," \
                       "DescriptorTable( SRV( t0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( SRV( t2, flags = DATA_VOLATILE ) )," \
                       "RootConstants( b1, num32BitConstants = 3 )," \
                       "DescriptorTable( SRV( t1, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )"

cbuffer Params : register( b1 )
{
  float2 invTexSize;
  float  bloomStrength;
};

Texture2D<float4>       source      : register( t0 );
Texture2D<float4>       bloom       : register( t1 );
Texture2D<float >       exposure    : register( t2 );
RWTexture2D<float4>     destination : register( u0 );

SamplerState linearSampler : register( s0 );

float3 TM_Reinhard( float3 hdr, float k = 1.0 )
{
  return hdr / ( hdr + k );
}

float3 TM_Stanard( float3 hdr )
{
  return TM_Reinhard( hdr * sqrt( hdr ), sqrt( 4.0 / 27.0 ) );
}

float3 ToneMapACESFilmic( float3 x )
{
  float a = 2.51f;
  float b = 0.03f;
  float c = 2.43f;
  float d = 0.59f;
  float e = 0.14f;
  return saturate( ( x * ( a * x + b ) ) / ( x * ( c * x + d ) + e ) );
}

float3 TosRGB( float3 c )
{
  return pow( abs( c ), 1.0 / 2.2 );
}

static const float gamma     = 2.2;
static const float pureWhite = 1.0;

[RootSignature( _RootSignature )]
[numthreads( ToneMappingKernelWidth, ToneMappingKernelHeight, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  float3 hdrColor = source[ dispatchThreadID.xy ].rgb;

  float3 color           = hdrColor * exposure[ int2( 0, 0 ) ];
  float3 bloomSample     = bloom.SampleLevel( linearSampler, ( dispatchThreadID.xy + 0.5 ) * invTexSize, 0 ).rgb;
  float  luminance       = dot( color, float3( 0.2126, 0.7152, 0.0722 ) );
  float  mappedLuminance = ( luminance * ( 1.0 + luminance / ( pureWhite * pureWhite ) ) ) / ( 1.0 + luminance );
  float3 mappedColor     = ( mappedLuminance / luminance ) * color + bloomSample * bloomStrength;

  destination[ dispatchThreadID.xy ] = float4( pow( mappedColor, 1.0 / gamma ), 1.0 );
}