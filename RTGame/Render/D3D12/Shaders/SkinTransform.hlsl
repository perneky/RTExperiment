#include "../../ShaderValues.h"
#include "RootSignatures/ShaderStructures.hlsli"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 2 )," \
                       "DescriptorTable( SRV( t0 ) )," \
                       "CBV( b1 )," \
                       "DescriptorTable( SRV( t2 ) )," \
                       "DescriptorTable( SRV( t3 ) )," \
                       "DescriptorTable( UAV( u0 ) )," \
                       "DescriptorTable( UAV( u1 ) )" \

cbuffer Params : register( b0 )
{
  int indexCount;
  int vertexCount;
};

cbuffer Params : register( b1 )
{
  matrix bones[ 256 ];
};

StructuredBuffer< uint >                           materials     : register( t0 );
StructuredBuffer< uint >                           indices       : register( t2 );
StructuredBuffer< SkinnedVertexFormat >            vertices      : register( t3 );
RWStructuredBuffer< RigidVertexFormatWithHistory > destination   : register( u0 );
RWStructuredBuffer< RTVertexFormat    >            rtDestination : register( u1 );

float2 HalfToFloat2( uint val )
{
  float2 result;
  result.x = f16tof32( val.x );
  result.y = f16tof32( val.x >> 16 );

  return result;
}

float3 HalfToFloat3( uint2 val )
{
  float3 result;
  result.x = f16tof32( val.x );
  result.y = f16tof32( val.x >> 16 );
  result.z = f16tof32( val.y );

  return result;
}

uint2 Float4ToHalf( float4 val )
{
  uint2 result;
  result.x  = f32tof16( val.x );
  result.x |= f32tof16( val.y ) << 16;
  result.y  = f32tof16( val.z );
  result.y |= f32tof16( val.w ) << 16;

  return result;
}

uint2 Float3ToHalf( float3 val )
{
  uint2 result;
  result.x  = f32tof16( val.x );
  result.x |= f32tof16( val.y ) << 16;
  result.y  = f32tof16( val.z );

  return result;
}

uint4 UnpackUint4( uint val )
{
  uint4 result;
  result.x = val >> 24;
  result.y = ( val >> 16 ) & 0xff;
  result.z = ( val >> 8  ) & 0xff;
  result.w = val & 0xff;
  return result;
}

float4 UnpackFloat4( uint val )
{
  return float4( UnpackUint4( val ) ) / 255;
}

[RootSignature( _RootSignature )]
[numthreads( SkinTransformKernelWidth, 1, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  [branch]
  if ( dispatchThreadID.x >= indexCount )
    return;

  uint index = indices[ dispatchThreadID.x ];

  [branch]
  if ( index >= vertexCount )
    return;

  SkinnedVertexFormat skin = vertices[ index ];

  RigidVertexFormatWithHistory rigid = destination[ index ];
  rigid.oldPosition = rigid.position;

  float3 position  = HalfToFloat3( skin.position );
  float3 normal    = HalfToFloat3( skin.normal );
  float3 tangent   = HalfToFloat3( skin.tangent );
  float3 bitangent = HalfToFloat3( skin.bitangent );
  uint4  indices   = UnpackUint4 ( skin.blendIndices );
  float4 weights   = UnpackFloat4( skin.blendWeights );

  float3 p = float3( 0, 0, 0 );
  float3 n = float3( 0, 0, 0 );
  float3 t = float3( 0, 0, 0 );
  float3 b = float3( 0, 0, 0 );

  [unroll]
  for ( uint ix = 0; ix < 4; ix++ )
  {
    p += mul( float4( position, 1 ), bones[ indices[ ix ] ] ).xyz * weights[ ix ];

    n += mul( normal,    ( float3x3 )bones[ indices[ ix ] ] ) * weights[ ix ];
    t += mul( tangent,   ( float3x3 )bones[ indices[ ix ] ] ) * weights[ ix ];
    b += mul( bitangent, ( float3x3 )bones[ indices[ ix ] ] ) * weights[ ix ];
  }

  rigid.position  = Float4ToHalf( float4( p, 1 ) );
  rigid.normal    = Float3ToHalf( n );
  rigid.tangent   = Float3ToHalf( t );
  rigid.bitangent = Float3ToHalf( b );
  rigid.texcoord  = skin.texcoord;

  RTVertexFormat rt;
  rt.normal        = n;
  rt.tangent       = t;
  rt.bitangent     = b;
  rt.texcoord      = HalfToFloat2( skin.texcoord );
  rt.materialIndex = materials[ index ];

  destination  [ index ] = rigid;
  rtDestination[ index ] = rt;
}