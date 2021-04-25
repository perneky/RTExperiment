#include "../../ShaderValues.h"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 2 )," \
                       "CBV( b1 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \

cbuffer Params : register( b0 )
{
  float2 inverseOutputSize;
}

cbuffer Exposure : register( b1 )
{
  ExposureBufferCB exposure;
}

SamplerState BiLinearClamp : register( s0 );

Texture2D< float3 > SourceTex : register( t0 );

RWTexture2D< uint > LumaResult : register( u0 );

float LinearToLogLuminance( float x, float gamma = 4.0 )
{
  return log2( lerp( 1, exp2( gamma ), x ) ) / gamma;
}

float RGBToLuminance( float3 x )
{
  return dot( x, float3( 0.212671, 0.715160, 0.072169 ) );
}

float RGBToLogLuminance( float3 x, float gamma = 4.0 )
{
  return LinearToLogLuminance( RGBToLuminance( x ), gamma );
}

[RootSignature( _RootSignature )]
[numthreads( ExtractLumaKernelWidth, ExtractLumaKernelHeight, 1 )]
void main( uint3 DTid : SV_DispatchThreadID )
{
  float2 uv     = DTid.xy * inverseOutputSize;
  float2 offset = inverseOutputSize * 0.25f;

  float luma1 = RGBToLuminance( SourceTex.SampleLevel( BiLinearClamp, uv + float2( -offset.x, -offset.y ), 0 ).rgb );
  float luma2 = RGBToLuminance( SourceTex.SampleLevel( BiLinearClamp, uv + float2(  offset.x, -offset.y ), 0 ).rgb );
  float luma3 = RGBToLuminance( SourceTex.SampleLevel( BiLinearClamp, uv + float2( -offset.x,  offset.y ), 0 ).rgb );
  float luma4 = RGBToLuminance( SourceTex.SampleLevel( BiLinearClamp, uv + float2(  offset.x,  offset.y ), 0 ).rgb );

  float luma = ( luma1 + luma2 + luma3 + luma4 ) * 0.25;

  if ( luma == 0.0 )
  {
    LumaResult[ DTid.xy ] = 0;
  }
  else
  {
    float MinLog      = exposure.minLog;
    float RcpLogRange = exposure.invLogRange;
    float logLuma     = saturate( ( log2( luma ) - MinLog ) * RcpLogRange );    // Rescale to [0.0, 1.0]
        
    LumaResult[ DTid.xy ] = logLuma * 254.0 + 1.0;                              // Rescale to [1, 255]
  }
}
