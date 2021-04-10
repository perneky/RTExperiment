#pragma once

#include "../../ShaderValues.h"
#include "MaterialUtils.hlsli"
#include "RTUtils.hlsli"

static const float giSampleMipLevel = 2;

float3 TraceDirectLighting( float3 albedo, float roughness, float metallic, bool isSpecular, float3 worldPosition, float3 worldNormal, float3 cameraPosition, LightingEnvironmentParamsCB env, bool noScatter );

int FilterGIProbes( float3 worldPosition, float3 worldNormal, FrameParamsCB frameParams, inout NearestProbes closest )
{
  int count = 8;
  for ( int ix = 0; ix < 8; ++ix )
  {
    float3 probePosition = GIProbePosition( closest.indices[ ix ].xyz, frameParams );
    float3 toProbe       = probePosition - worldPosition;
    float4 plane         = PlaneFromPointNormal( worldPosition, worldNormal );
    float  distance      = PointDistanceFromPlane( probePosition, plane );

    // If the distance from the polygon plane is too small,
    // then there is a higher chance that the ray is going
    // to collide with the current triangle.
    // To prevent this, we discard probes which are too close
    // to the plane. If no probes found, close probes will be
    // reused in the next pass.
    [branch]
    if ( distance < 0.01 )
    {
      closest.indices[ ix ] = -1;
      count--;
      continue;
    }

    HitGeometry hitGeom = TraceRay( worldPosition + worldNormal * 0.01, toProbe, 0.002, 1 );

    if ( hitGeom.t >= 0 && IsOpaqueMaterial( hitGeom.materialIndex ) )
    {
      closest.indices[ ix ].w = -1;
      count--;
    }
  }
  
  return count;
}

int FilterGIProbesWORT( float3 worldPosition, float3 worldNormal, FrameParamsCB frameParams, inout NearestProbes closest )
{
  int count = 8;
  for ( int ix = 0; ix < 8; ++ix )
  {
    float3 probePosition = GIProbePosition( closest.indices[ ix ].xyz, frameParams );
    float4 plane         = PlaneFromPointNormal( worldPosition, worldNormal );
    float  distance      = PointDistanceFromPlane( probePosition, plane );

    // As there are no rays traced, any probe will do
    // in front of the triangle, we don't need that
    // small treshold.
    if ( distance < 0 )
    {
      closest.indices[ ix ] = -1;
      count--;
    }
  }

  return count;
}

static const int3 giNeighbors[ 8 ] =
{
  int3( 1, 2, 4 ),
  int3( 0, 3, 5 ),
  int3( 0, 3, 6 ),
  int3( 1, 2, 7 ),
  int3( 0, 5, 6 ),
  int3( 1, 4, 7 ),
  int3( 2, 4, 7 ),
  int3( 3, 5, 6 ),
};

float3 InterpolateGI( NearestProbes closest, float3 worldPosition, FrameParamsCB frameParams, Texture3D probeTexture )
{
  float3 gi[ 8 ];

  float3 result = 0;
  for ( int item = 0; item < 8; ++item )
  {
    int4 probeLocation = closest.indices[ item ];

    [branch]
    if ( probeLocation.w < 0 )
      gi[ item ] = -1;
    else
      gi[ item ] = probeTexture.Load( int4( probeLocation.xyz, 0 ) ).rgb;
  }

  // Fill the missing samples from their valid neighbours
  [unroll]
  for ( int iter = 0; iter < 4; ++iter )
    if ( all( gi[ iter ] < 0 ) )
      gi[ iter ] = gi[ iter + 4 ];

  [unroll]
  for ( iter = 4; iter < 8; ++iter )
    if ( all( gi[ iter ] < 0 ) )
      gi[ iter ] = gi[ iter - 4 ];

  if ( all( gi[ 0 ] < 0 ) )
    gi[ 0 ] = gi[ 1 ];
  if ( all( gi[ 2 ] < 0 ) )
    gi[ 2 ] = gi[ 3 ];
  if ( all( gi[ 4 ] < 0 ) )
    gi[ 4 ] = gi[ 5 ];
  if ( all( gi[ 6 ] < 0 ) )
    gi[ 6 ] = gi[ 7 ];

  if ( all( gi[ 1 ] < 0 ) )
    gi[ 1 ] = gi[ 0 ];
  if ( all( gi[ 3 ] < 0 ) )
    gi[ 3 ] = gi[ 2 ];
  if ( all( gi[ 5 ] < 0 ) )
    gi[ 5 ] = gi[ 4 ];
  if ( all( gi[ 7 ] < 0 ) )
    gi[ 7 ] = gi[ 6 ];

  if ( all( gi[ 0 ] < 0 ) )
    gi[ 0 ] = gi[ 2 ];
  if ( all( gi[ 1 ] < 0 ) )
    gi[ 1 ] = gi[ 3 ];
  if ( all( gi[ 4 ] < 0 ) )
    gi[ 4 ] = gi[ 6 ];
  if ( all( gi[ 5 ] < 0 ) )
    gi[ 5 ] = gi[ 7 ];

  if ( all( gi[ 2 ] < 0 ) )
    gi[ 2 ] = gi[ 0 ];
  if ( all( gi[ 3 ] < 0 ) )
    gi[ 3 ] = gi[ 1 ];
  if ( all( gi[ 6 ] < 0 ) )
    gi[ 6 ] = gi[ 4 ];
  if ( all( gi[ 7 ] < 0 ) )
    gi[ 7 ] = gi[ 5 ];

  float3 o = worldPosition - frameParams.giProbeOrigin.xyz;
  float3 s = o / GIProbeSpacing;
  float3 uvw = frac( s );

  float3 d1 = lerp( gi[ 0 ], gi[ 1 ], uvw.x );
  float3 d2 = lerp( gi[ 2 ], gi[ 3 ], uvw.x );
  float3 u1 = lerp( gi[ 4 ], gi[ 5 ], uvw.x );
  float3 u2 = lerp( gi[ 6 ], gi[ 7 ], uvw.x );
  float3 dc = lerp( d1, d2, uvw.z );
  float3 uc = lerp( u1, u2, uvw.z );
  float3 cc = lerp( dc, uc, uvw.y );

  return max( cc, 0 );
}

float3 SampleSecondGI( float3 worldPosition, float3 worldNormal, float3 firstWorldPosition, float3 firstWorldNormal, FrameParamsCB frameParams, Texture3D probeTexture )
{
  NearestProbes closest = GetNearestProbes( worldPosition, frameParams );
  int           items   = FilterGIProbes( worldPosition, worldNormal, frameParams, closest );

  [branch]
  if ( items == 0 )
  {
    closest = GetNearestProbes( firstWorldPosition, frameParams );
    items   = FilterGIProbesWORT( firstWorldPosition, firstWorldNormal, frameParams, closest );
    return InterpolateGI( closest, firstWorldPosition, frameParams, probeTexture );
  }

  return InterpolateGI( closest, worldPosition, frameParams, probeTexture );
}

float3 SampleGI( float3 worldPosition, float3 worldNormal, FrameParamsCB frameParams, Texture3D probeTexture )
{
  NearestProbes closest = GetNearestProbes( worldPosition, frameParams );
  int           items   = FilterGIProbes( worldPosition, worldNormal, frameParams, closest );

  [branch]
  if ( items == 0 )
  {
    HitGeometry hitGeom = TraceRay( worldPosition, worldNormal, castMinDistance, 1 );

    [branch]
    if ( hitGeom.t >= 0 )
      return SampleSecondGI( hitGeom.worldPosition, hitGeom.worldNormal, worldPosition, worldNormal, frameParams, probeTexture );
    else
    {
      closest = GetNearestProbes( worldPosition, frameParams );
      items = FilterGIProbesWORT( worldPosition, worldNormal, frameParams, closest );
    }
  }

  return InterpolateGI( closest, worldPosition, frameParams, probeTexture );
}

bool TraceOpaquePosition( float3 worldPosition, float3 worldDirection, out float visibility, out HitGeometry hitGeom )
{
  visibility = 1;

  hitGeom = (HitGeometry)0;

  float3 origin = worldPosition;

  while ( visibility > 0 )
  {
    hitGeom = TraceRay( origin, worldDirection, castMinDistance, 1000 );

    [branch]
    if ( hitGeom.t >= 0 )
    {
      [branch]
      if ( IsOpaqueMaterial( hitGeom.materialIndex ) )
        return true;

      float4 surfaceAlbedoAlpha = SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord, giSampleMipLevel );

      [branch]
      if ( IsOneBitAlphaMaterial( hitGeom.materialIndex ) && !ShouldBeDiscared( surfaceAlbedoAlpha.a ) )
        return true;

      // Only consider translucent surfaces, facing towards the probe, or two sided
      [branch]
      if ( IsTwoSidedMaterial( hitGeom.materialIndex ) || dot( hitGeom.worldNormal, worldDirection ) < 0 )
      {
        visibility -= surfaceAlbedoAlpha.a;

        [branch]
        if ( visibility <= 0 )
        {
          visibility += surfaceAlbedoAlpha.a;
          return true;
        }
      }

      origin = hitGeom.worldPosition + worldDirection * castMinDistance;
    }
    else
      break;
  }

  return false;
}

float3 CalcGI( HitGeometry hitGeom, LightingEnvironmentParamsCB env, FrameParamsCB frameParams )
{
  float4 surfaceAlbedoAlpha = SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord, giSampleMipLevel );
  float  roughness          = SampleRoughness( hitGeom.materialIndex, hitGeom.texcoord, giSampleMipLevel );
  float  metallic           = SampleMetallic( hitGeom.materialIndex, hitGeom.texcoord, giSampleMipLevel );
  bool   isSpecular         = IsSpecularMaterial( hitGeom.materialIndex );

  float3 directLighting = TraceDirectLighting( surfaceAlbedoAlpha.rgb, roughness, metallic, isSpecular, hitGeom.worldPosition, hitGeom.worldNormal, frameParams.cameraPosition.xyz, env, true );

  return directLighting;
}

float3 TraceGI( float3 worldPosition, float3 worldDirection, LightingEnvironmentParamsCB env, FrameParamsCB frameParams )
{
  float       visibility;
  HitGeometry hitGeom = (HitGeometry)0;

  [branch]
  if ( TraceOpaquePosition( worldPosition, worldDirection, visibility, hitGeom ) )
    return CalcGI( hitGeom, env, frameParams ) * visibility;
  else
    return allCubeTextures[ allMaterials.mat[ env.skyMaterial ].textureIndices.y ].SampleLevel( wrapSampler, worldDirection, 0 ).rgb * visibility;
}
