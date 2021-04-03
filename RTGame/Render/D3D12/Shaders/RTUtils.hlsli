#pragma once

static const uint QueryFlags = 0;
typedef RayQuery< QueryFlags > Query;

static const float castMinDistance = 0.0001f;

struct HitGeometry
{
  float3 worldPosition;
  float3 worldNormal;
  float3 worldTangent;
  float3 worldBitangent;
  float2 texcoord;
  int    materialIndex;
  float  t;
};

static const HitGeometry hitGeomMiss = { 0, 0, 0, /**/ 0, 0, 0, /***/ 0, 0, 0, /**/ 0, 0, 0, /**/ 0, 0, /**/ 0, /**/ -1 };

HitGeometry CalcHitGeometry( uint instanceId, uint triangleIndex, float2 barycentrics2, float hitDistance, float3x4 toWorld, float3 worldRayOrigin, float3 worldRayDirection )
{
  HitGeometry result;

  const uint indexSize = 2;

  uint ibSlotIndex = ( instanceId >> 12 ) & RTSlotMask;
  uint vbSlotIndex = instanceId & RTSlotMask;

  ByteAddressBuffer meshIB  = meshIndices[ ibSlotIndex ];
  uint3             indices = Load3x16BitIndices( meshIB, triangleIndex * indexSize * 3 );

  StructuredBuffer< RTVertexFormat > meshVB = meshVertices[ vbSlotIndex ];
  RTVertexFormat v1 = meshVB[ indices[ 0 ] ];
  RTVertexFormat v2 = meshVB[ indices[ 1 ] ];
  RTVertexFormat v3 = meshVB[ indices[ 2 ] ];

  float3 barycentrics = float3( 1.0 - barycentrics2.x - barycentrics2.y, barycentrics2.x, barycentrics2.y );

  float3 normal = v1.normal * barycentrics.x 
                + v2.normal * barycentrics.y
                + v3.normal * barycentrics.z;
  float3 tangent = v1.tangent * barycentrics.x 
                 + v2.tangent * barycentrics.y
                 + v3.tangent * barycentrics.z;
  float3 bitangent = v1.bitangent * barycentrics.x 
                   + v2.bitangent * barycentrics.y
                   + v3.bitangent * barycentrics.z;

  result.materialIndex  = v1.materialIndex;
  result.worldNormal    = mul( ( float3x3 )toWorld, normal );
  result.worldTangent   = mul( ( float3x3 )toWorld, tangent );
  result.worldBitangent = mul( ( float3x3 )toWorld, bitangent );
  result.worldPosition  = worldRayOrigin + worldRayDirection * hitDistance;
  result.texcoord       = v1.texcoord * barycentrics.x
                        + v2.texcoord * barycentrics.y
                        + v3.texcoord * barycentrics.z;

  if ( IsScaleUVMaterial( result.materialIndex ) )
    result.texcoord *= length( float3( toWorld._11, toWorld._22, toWorld._33 ) );

  result.t = hitDistance;

  return result;
}

HitGeometry CalcHitGeometry( Query query )
{
  return CalcHitGeometry( query.CommittedInstanceID()
                        , query.CommittedPrimitiveIndex()
                        , query.CommittedTriangleBarycentrics()
                        , query.CommittedRayT()
                        , query.CommittedObjectToWorld3x4()
                        , query.WorldRayOrigin()
                        , query.WorldRayDirection() );
}

HitGeometry TraceRay( float3 origin
                    , float3 direction
                    , float tmin
                    , float tmax )
{
#if ENABLE_RAYTRACING_FOR_RENDER
  Query   query;
  RayDesc ray;

  ray.Origin    = origin;
  ray.Direction = direction;
  ray.TMin      = tmin;
  ray.TMax      = tmax;

  query.TraceRayInline( rayTracingScenes[ 0 ], QueryFlags, 0xFF, ray );
  query.Proceed();
  
  [branch]
  if ( query.CommittedStatus() == COMMITTED_TRIANGLE_HIT )
    return CalcHitGeometry( query );

  return hitGeomMiss;
#else
  return hitGeomMiss;
#endif // ENABLE_RAYTRACING_FOR_RENDER
}