#pragma once

#include "RootSignatures/ShaderStructures.hlsli"

float4 PlaneFromPointNormal( float3 position, float3 normal )
{
  return float4( normal, -dot( position, normal ) );
}

float PointDistanceFromPlane( float3 position, float4 plane )
{
  return dot( position, plane.xyz ) + plane.w;
}

float3 FromsRGB( float3 c )
{
  return pow( c, 2.2 );
}

float3 GetNextRandomOffset( Texture3D randomTexture, SamplerState randomSampler, float3 randomOffset, inout float3 randomValues )
{
  static const float randomScale = 128;

  float3 offset = randomTexture.SampleLevel( randomSampler, frac( randomValues * randomScale + randomOffset ), 0 ).rgb * 2 - 1;
  randomValues += offset;
  return offset;
}

uint3 Load3x16BitIndices( ByteAddressBuffer indexBuffer, uint offsetBytes )
{
  uint3 indices;

  uint  dwordAlignedOffset = offsetBytes & ~3;
  uint2 four16BitIndices   = indexBuffer.Load2( dwordAlignedOffset );

  if ( dwordAlignedOffset == offsetBytes )
  {
    indices.x = four16BitIndices.x & 0xffff;
    indices.y = ( four16BitIndices.x >> 16 ) & 0xffff;
    indices.z = four16BitIndices.y & 0xffff;
  }
  else
  {
    indices.x = ( four16BitIndices.x >> 16 ) & 0xffff;
    indices.y = four16BitIndices.y & 0xffff;
    indices.z = ( four16BitIndices.y >> 16 ) & 0xffff;
  }

  return indices;
}

uint Load16BitIndex( ByteAddressBuffer indexBuffer, uint offsetBytes )
{
  uint dwordAlignedOffset = offsetBytes & ~3;
  uint two16BitIndices    = indexBuffer.Load( dwordAlignedOffset );

  if ( dwordAlignedOffset == offsetBytes )
    return two16BitIndices & 0xffff;
  else
    return ( two16BitIndices >> 16 ) & 0xffff;
}

float LinearizeDepth( float nonLinearDepth, matrix invProj )
{
  float4 ndcCoords  = float4( 0, 0, nonLinearDepth, 1.0f );
  float4 viewCoords = mul( invProj, ndcCoords );
  return viewCoords.z / viewCoords.w;
}

bool ShouldBeDiscared( float alpha )
{
  return alpha < 1.0 / 255;
}

void CalcAxesForVector( float3 v, out float3 a1, out float3 a2 )
{
  float3 axis;
  if ( abs( v.y ) < 0.9 )
    axis = float3( 0, 1, 0 );
  else
    axis = float3( 1, 0, 0 );

  a1 = cross( axis, v );
  a2 = cross( v, a1 );
}

float3 PertubNormal( float3 normal, float3 px, float3 py, float3 offset )
{
  return normalize( px * offset.x + py * offset.y + normal * ( abs( offset.z ) * 0.9 + 0.1 ) );
}

float3 PertubPosition( float3 position, float3 px, float3 py, float2 offset )
{
  return position + ( px * offset.x + py * offset.y );
}

float3 UVToDirectionSpherical( float2 uv )
{
  uv.x *= PI;
  uv.y *= PIPI;
  uv.y -= PI;

  float2 s, c;
  sincos( uv, s, c );

  float x = s.x * c.y;
  float y = s.x * s.y;
  float z = c.x;
  return float3( x, y, z );
}

float3 UVToDirectionOctahedral( float2 f )
{
  f = f * 2.0 - 1.0;

  // https://twitter.com/Stubbesaurus/status/937994790553227264
  float3 n = float3( f.x, f.y, 1.0 - abs( f.x ) - abs( f.y ) );
  float t = saturate( -n.z );
  n.xy += n.xy >= 0.0 ? -t : t;
  return normalize( n );
}

int3 GIProbeWorldIndex( uint index, FrameParamsCB frameParams )
{
  int x = index % frameParams.giProbeCount.x;
  int y = index / frameParams.giProbeCount.w;
  int z = ( index % frameParams.giProbeCount.w ) / frameParams.giProbeCount.x;

  return int3( x, y, z );
}

float3 GIProbePosition( uint3 location, FrameParamsCB frameParams )
{
  return frameParams.giProbeOrigin.xyz + float3( location ) * GIProbeSpacing;
}

float3 GIProbePosition( uint index, FrameParamsCB frameParams )
{
  float x = index % frameParams.giProbeCount.x;
  float y = index / frameParams.giProbeCount.w;
  float z = ( index % frameParams.giProbeCount.w ) / frameParams.giProbeCount.x;

  return frameParams.giProbeOrigin.xyz + float3( x, y, z ) * GIProbeSpacing;
}

int GIProbeIndex( uint3 index, FrameParamsCB frameParams )
{
  return index.y * frameParams.giProbeCount.w + index.z * frameParams.giProbeCount.x + index.x;
}

struct NearestProbes
{
  int4 indices[ 8 ];
};

NearestProbes NP( int3 i )
{
  NearestProbes np;
  np.indices[ 0 ] = np.indices[ 1 ] = np.indices[ 2 ] = np.indices[ 3 ] =
  np.indices[ 4 ] = np.indices[ 5 ] = np.indices[ 6 ] = np.indices[ 7 ] = int4( i, 1 );
  return np;
}

NearestProbes GetNearestProbes( float3 worldPosition, FrameParamsCB frameParams )
{
  float3 o = worldPosition - frameParams.giProbeOrigin.xyz;
  float3 s = o / GIProbeSpacing;
  int3   i = max( 0, min( floor( s ), frameParams.giProbeCount.xyz - 1 ) );

  NearestProbes np = NP( i );

  bool3 b = i < ( frameParams.giProbeCount.xyz - 1 );

  if ( b.x )
  {
    np.indices[ 1 ].x++;
    np.indices[ 3 ].x++;
    np.indices[ 5 ].x++;
    np.indices[ 7 ].x++;
  }
  if ( b.z )
  {
    np.indices[ 2 ].z++;
    np.indices[ 3 ].z++;
    np.indices[ 6 ].z++;
    np.indices[ 7 ].z++;
  }
  if ( b.y )
  {
    np.indices[ 4 ].y++;
    np.indices[ 5 ].y++;
    np.indices[ 6 ].y++;
    np.indices[ 7 ].y++;
  }

  return np;
}

float Lightness( float3 c )
{
  return dot( c, float3( 0.212671, 0.715160, 0.072169 ) );
}

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
  result.x = f32tof16( val.x );
  result.x |= f32tof16( val.y ) << 16;
  result.y = f32tof16( val.z );
  result.y |= f32tof16( val.w ) << 16;

  return result;
}

uint2 Float3ToHalf( float3 val )
{
  uint2 result;
  result.x = f32tof16( val.x );
  result.x |= f32tof16( val.y ) << 16;
  result.y = f32tof16( val.z );

  return result;
}

uint4 UnpackUint4( uint val )
{
  uint4 result;
  result.x = val >> 24;
  result.y = ( val >> 16 ) & 0xff;
  result.z = ( val >> 8 ) & 0xff;
  result.w = val & 0xff;
  return result;
}

float4 UnpackFloat4( uint val )
{
  return float4( UnpackUint4( val ) ) / 255;
}
