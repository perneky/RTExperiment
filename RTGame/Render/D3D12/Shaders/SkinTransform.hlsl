#include "../../ShaderValues.h"
#include "RootSignatures/ShaderStructures.hlsli"
#include "Utils.hlsli"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 2 )," \
                       "CBV( b1 )," \
                       "DescriptorTable( SRV( t2 ) )," \
                       "DescriptorTable( SRV( t3 ) )," \
                       "DescriptorTable( UAV( u0 ) )" \

cbuffer Params : register( b0 )
{
  int indexCount;
  int vertexCount;
};

cbuffer Params : register( b1 )
{
  matrix bones[ 256 ];
};

ByteAddressBuffer                                  indices     : register( t2 );
StructuredBuffer< SkinnedVertexFormat >            vertices    : register( t3 );
RWStructuredBuffer< RigidVertexFormatWithHistory > destination : register( u0 );

[RootSignature( _RootSignature )]
[numthreads( SkinTransformKernelWidth, 1, 1 )]
void main( uint3 dispatchThreadID : SV_DispatchThreadID )
{
  uint index = Load16BitIndex( indices, dispatchThreadID.x * 2 );

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

  destination[ index ] = rigid;
}