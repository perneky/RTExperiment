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

  BLASGPUInfo blasInfo = blasGPUInfo[ instanceId ];

  uint ibSlotIndex = blasInfo.indexBufferId;
  uint vbSlotIndex = blasInfo.vertexBufferId;

  ByteAddressBuffer meshIB  = meshIndices[ ibSlotIndex ];
  uint3             indices = Load3x16BitIndices( meshIB, triangleIndex * indexSize * 3 );

  float3 normals   [ 3 ];
  float3 tangents  [ 3 ];
  float3 bitangents[ 3 ];
  float2 texcoords [ 3 ];

  [branch]
  if ( ( blasInfo.flags & (uint)BLASGPUInfoFlags::HasHistory ) != 0 )
  {
    RigidVertexFormatWithHistory v1 = meshVerticesWH[ vbSlotIndex ][ indices.x ];
    RigidVertexFormatWithHistory v2 = meshVerticesWH[ vbSlotIndex ][ indices.y ];
    RigidVertexFormatWithHistory v3 = meshVerticesWH[ vbSlotIndex ][ indices.z ];
    normals   [ 0 ] = HalfToFloat3( v1.normal    );
    normals   [ 1 ] = HalfToFloat3( v2.normal    );
    normals   [ 2 ] = HalfToFloat3( v3.normal    );
    tangents  [ 0 ] = HalfToFloat3( v1.tangent   );
    tangents  [ 1 ] = HalfToFloat3( v2.tangent   );
    tangents  [ 2 ] = HalfToFloat3( v3.tangent   );
    bitangents[ 0 ] = HalfToFloat3( v1.bitangent );
    bitangents[ 1 ] = HalfToFloat3( v2.bitangent );
    bitangents[ 2 ] = HalfToFloat3( v3.bitangent );
    texcoords [ 0 ] = HalfToFloat2( v1.texcoord  );
    texcoords [ 1 ] = HalfToFloat2( v2.texcoord  );
    texcoords [ 2 ] = HalfToFloat2( v3.texcoord  );
  }
  else
  {
    RigidVertexFormat v1 = meshVertices[ vbSlotIndex ][ indices.x ];
    RigidVertexFormat v2 = meshVertices[ vbSlotIndex ][ indices.y ];
    RigidVertexFormat v3 = meshVertices[ vbSlotIndex ][ indices.z ];
    normals   [ 0 ] = HalfToFloat3( v1.normal    );
    normals   [ 1 ] = HalfToFloat3( v2.normal    );
    normals   [ 2 ] = HalfToFloat3( v3.normal    );
    tangents  [ 0 ] = HalfToFloat3( v1.tangent   );
    tangents  [ 1 ] = HalfToFloat3( v2.tangent   );
    tangents  [ 2 ] = HalfToFloat3( v3.tangent   );
    bitangents[ 0 ] = HalfToFloat3( v1.bitangent );
    bitangents[ 1 ] = HalfToFloat3( v2.bitangent );
    bitangents[ 2 ] = HalfToFloat3( v3.bitangent );
    texcoords [ 0 ] = HalfToFloat2( v1.texcoord  );
    texcoords [ 1 ] = HalfToFloat2( v2.texcoord  );
    texcoords [ 2 ] = HalfToFloat2( v3.texcoord  );
  }

  float3 barycentrics = float3( 1.0 - barycentrics2.x - barycentrics2.y, barycentrics2.x, barycentrics2.y );

  float3 normal = normals[ 0 ] * barycentrics.x
                + normals[ 1 ] * barycentrics.y
                + normals[ 2 ] * barycentrics.z;
  float3 tangent = tangents[ 0 ] * barycentrics.x
                 + tangents[ 1 ] * barycentrics.y
                 + tangents[ 2 ] * barycentrics.z;
  float3 bitangent = bitangents[ 0 ] * barycentrics.x
                   + bitangents[ 1 ] * barycentrics.y
                   + bitangents[ 2 ] * barycentrics.z;

  result.materialIndex  = blasInfo.materialId;
  result.worldNormal    = mul( ( float3x3 )toWorld, normal );
  result.worldTangent   = mul( ( float3x3 )toWorld, tangent );
  result.worldBitangent = mul( ( float3x3 )toWorld, bitangent );
  result.worldPosition  = worldRayOrigin + worldRayDirection * hitDistance;
  result.texcoord       = texcoords[ 0 ] * barycentrics.x
                        + texcoords[ 1 ] * barycentrics.y
                        + texcoords[ 2 ] * barycentrics.z;

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

HitGeometry TraceRay( float3 origin, float3 direction, float tmin, float tmax )
{
#if ENABLE_RAYTRACING_FOR_RENDER
  Query   query;
  RayDesc ray;

  ray.Origin    = origin;
  ray.Direction = direction;
  ray.TMin      = tmin;
  ray.TMax      = tmax;

  query.TraceRayInline( rayTracingScene, QueryFlags, 0xFF, ray );
  query.Proceed();
  
  [branch]
  if ( query.CommittedStatus() == COMMITTED_TRIANGLE_HIT )
    return CalcHitGeometry( query );

  return hitGeomMiss;
#else
  return hitGeomMiss;
#endif // ENABLE_RAYTRACING_FOR_RENDER
}

bool CanAccess( float3 origin, float3 direction, float tmin, float tmax )
{
#if ENABLE_RAYTRACING_FOR_RENDER

  const uint QueryFlags = RAY_FLAG_CULL_NON_OPAQUE | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH;

  RayQuery< QueryFlags > query;
  RayDesc                ray;

  ray.Origin    = origin;
  ray.Direction = direction;
  ray.TMin      = tmin;
  ray.TMax      = tmax;

  query.TraceRayInline( rayTracingScene, QueryFlags, 0xFF, ray );
  query.Proceed();

  return query.CommittedStatus() != COMMITTED_TRIANGLE_HIT;
#else
  return true;
#endif // ENABLE_RAYTRACING_FOR_RENDER
}