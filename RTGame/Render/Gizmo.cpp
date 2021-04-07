#include "Gizmo.h"
#include "ShaderStructuresNative.h"
#include "Resource.h"
#include "RenderManager.h"
#include "Camera.h"
#include "Utils.h"

static const float r1 = 0.1f;
static const float r2 = 0.2f;
static const float l0 = r1;
static const float l1 = 0.8f;
static const float l2 = 0.5f;
static const float lt = l1 + l2;
static const float ps = l1;

static const float sc = l1;

static const float ci = lt;
static const float co = ci * 1.2f;

#define CNT index++

Gizmo::Gizmo( CommandList& commandList, GizmoType type )
  : type( type )
{
  switch ( type )
  {
  case GizmoType::Move:
    CreateMoveGeometry( commandList );
    break;
  case GizmoType::Rotate:
    CreateRotateGeometry( commandList );
    break;
  case GizmoType::Scale:
    CreateScaleGeometry( commandList );
    break;
  default:
    assert( false );
  }

  auto& device = RenderManager::GetInstance().GetDevice();
  constantBuffer = device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Default, false, sizeof( MeshParamsCB ), 0, L"MeshCB" );
}

Gizmo::~Gizmo()
{
}

void Gizmo::TearDown( CommandList& commandList )
{
  commandList.HoldResource( std::move( vertexBuffer ) );
  commandList.HoldResource( std::move( indexBuffer ) );
  commandList.HoldResource( std::move( constantBuffer ) );
}

GizmoType Gizmo::GetType() const
{
  return type;
}

void Gizmo::Render( CommandList& commandList, FXMMATRIX wvpTransformNoJitter )
{
  commandList.SetVertexBuffer( *vertexBuffer );
  commandList.SetIndexBuffer( *indexBuffer );

  MeshParamsCB meshParams;
  XMStoreFloat4x4( &meshParams.wvpTransform, wvpTransformNoJitter );

  auto uploadConstantBuffer = RenderManager::GetInstance().GetUploadConstantBufferForResource( *constantBuffer );
  commandList.UploadBufferResource( std::move( uploadConstantBuffer ), *constantBuffer, &meshParams, sizeof( MeshParamsCB ) );
  commandList.SetConstantBuffer( 1, *constantBuffer );

  commandList.SetPrimitiveType( primitiveType );
  commandList.DrawIndexed( indexCount );
}

GizmoElement Gizmo::Pick( FXMMATRIX xform, FXMVECTOR rayOrigin, FXMVECTOR rayDirection ) const
{
  switch ( type )
  {
  case GizmoType::Move:
    return PickMove( xform, rayOrigin, rayDirection );
  case GizmoType::Scale:
    return PickScale( xform, rayOrigin, rayDirection );
  case GizmoType::Rotate:
    return PickRotate( xform, rayOrigin, rayDirection );
  default:
    assert( false );
    return GizmoElement::None;
  }
}

void Gizmo::CreateMoveGeometry( CommandList& commandList )
{
  int index = 0;

  const size_t arrowVertexCount = 13;
  const size_t planeVertexCount = 4;
  GizmoVertexFormat vertices[ arrowVertexCount * 3 + planeVertexCount * 3 ];

  vertices[ CNT ] = { { l0, -r1,  r1 }, 0x010000FF };
  vertices[ CNT ] = { { l0, -r1, -r1 }, 0x010000FF };
  vertices[ CNT ] = { { l0,  r1, -r1 }, 0x010000FF };
  vertices[ CNT ] = { { l0,  r1,  r1 }, 0x010000FF };
                                       
  vertices[ CNT ] = { { l1, -r1,  r1 }, 0x010000FF };
  vertices[ CNT ] = { { l1, -r1, -r1 }, 0x010000FF };
  vertices[ CNT ] = { { l1,  r1, -r1 }, 0x010000FF };
  vertices[ CNT ] = { { l1,  r1,  r1 }, 0x010000FF };
                                       
  vertices[ CNT ] = { { l1, -r2,  r2 }, 0x010000FF };
  vertices[ CNT ] = { { l1, -r2, -r2 }, 0x010000FF };
  vertices[ CNT ] = { { l1,  r2, -r2 }, 0x010000FF };
  vertices[ CNT ] = { { l1,  r2,  r2 }, 0x010000FF };
  vertices[ CNT ] = { { lt,   0,   0 }, 0x010000FF };

  ///////////////////////////////////////

  vertices[ CNT ] = { { -r1, l0,  r1 }, 0x0200FF00 };
  vertices[ CNT ] = { { -r1, l0, -r1 }, 0x0200FF00 };
  vertices[ CNT ] = { {  r1, l0, -r1 }, 0x0200FF00 };
  vertices[ CNT ] = { {  r1, l0,  r1 }, 0x0200FF00 };

  vertices[ CNT ] = { { -r1, l1,  r1 }, 0x0200FF00 };
  vertices[ CNT ] = { { -r1, l1, -r1 }, 0x0200FF00 };
  vertices[ CNT ] = { {  r1, l1, -r1 }, 0x0200FF00 };
  vertices[ CNT ] = { {  r1, l1,  r1 }, 0x0200FF00 };

  vertices[ CNT ] = { { -r2, l1,  r2 }, 0x0200FF00 };
  vertices[ CNT ] = { { -r2, l1, -r2 }, 0x0200FF00 };
  vertices[ CNT ] = { {  r2, l1, -r2 }, 0x0200FF00 };
  vertices[ CNT ] = { {  r2, l1,  r2 }, 0x0200FF00 };
  vertices[ CNT ] = { {   0, lt,   0 }, 0x0200FF00 };

  ///////////////////////////////////////

  vertices[ CNT ] = { { -r1,  r1, l0 }, 0x04FF0000 };
  vertices[ CNT ] = { { -r1, -r1, l0 }, 0x04FF0000 };
  vertices[ CNT ] = { {  r1, -r1, l0 }, 0x04FF0000 };
  vertices[ CNT ] = { {  r1,  r1, l0 }, 0x04FF0000 };

  vertices[ CNT ] = { { -r1,  r1, l1 }, 0x04FF0000 };
  vertices[ CNT ] = { { -r1, -r1, l1 }, 0x04FF0000 };
  vertices[ CNT ] = { {  r1, -r1, l1 }, 0x04FF0000 };
  vertices[ CNT ] = { {  r1,  r1, l1 }, 0x04FF0000 };

  vertices[ CNT ] = { { -r2,  r2, l1 }, 0x04FF0000 };
  vertices[ CNT ] = { { -r2, -r2, l1 }, 0x04FF0000 };
  vertices[ CNT ] = { {  r2, -r2, l1 }, 0x04FF0000 };
  vertices[ CNT ] = { {  r2,  r2, l1 }, 0x04FF0000 };
  vertices[ CNT ] = { {   0,   0, lt }, 0x04FF0000 };

  ///////////////////////////////////////

  vertices[ CNT ] = { { r1, r1, 0 }, 0x0300FFFF };
  vertices[ CNT ] = { { ps, r1, 0 }, 0x0300FFFF };
  vertices[ CNT ] = { { ps, ps, 0 }, 0x0300FFFF };
  vertices[ CNT ] = { { r1, ps, 0 }, 0x0300FFFF };

  ///////////////////////////////////////

  vertices[ CNT ] = { { r1, 0, r1 }, 0x05FF00FF };
  vertices[ CNT ] = { { ps, 0, r1 }, 0x05FF00FF };
  vertices[ CNT ] = { { ps, 0, ps }, 0x05FF00FF };
  vertices[ CNT ] = { { r1, 0, ps }, 0x05FF00FF };

  ///////////////////////////////////////

  vertices[ CNT ] = { { 0, r1, r1 }, 0x06FFFF00 };
  vertices[ CNT ] = { { 0, ps, r1 }, 0x06FFFF00 };
  vertices[ CNT ] = { { 0, ps, ps }, 0x06FFFF00 };
  vertices[ CNT ] = { { 0, r1, ps }, 0x06FFFF00 };

  const size_t arrowIndexCount = 36 + 12 + 6;
  const size_t planeIndexCount = 6;
  uint16_t indices[ arrowIndexCount * 3 + planeIndexCount * 3 ];

  index = 0;

#define GAI( base ) \
  indices[ CNT ] = 0 + base; \
  indices[ CNT ] = 1 + base; \
  indices[ CNT ] = 2 + base; \
  indices[ CNT ] = 2 + base; \
  indices[ CNT ] = 3 + base; \
  indices[ CNT ] = 0 + base; \
  indices[ CNT ] = 1 + base; \
  indices[ CNT ] = 5 + base; \
  indices[ CNT ] = 6 + base; \
  indices[ CNT ] = 6 + base; \
  indices[ CNT ] = 2 + base; \
  indices[ CNT ] = 1 + base; \
  indices[ CNT ] = 0 + base; \
  indices[ CNT ] = 4 + base; \
  indices[ CNT ] = 5 + base; \
  indices[ CNT ] = 5 + base; \
  indices[ CNT ] = 1 + base; \
  indices[ CNT ] = 0 + base; \
  indices[ CNT ] = 5 + base; \
  indices[ CNT ] = 4 + base; \
  indices[ CNT ] = 7 + base; \
  indices[ CNT ] = 7 + base; \
  indices[ CNT ] = 6 + base; \
  indices[ CNT ] = 5 + base; \
  indices[ CNT ] = 4 + base; \
  indices[ CNT ] = 0 + base; \
  indices[ CNT ] = 3 + base; \
  indices[ CNT ] = 3 + base; \
  indices[ CNT ] = 7 + base; \
  indices[ CNT ] = 4 + base; \
  indices[ CNT ] = 2 + base; \
  indices[ CNT ] = 6 + base; \
  indices[ CNT ] = 7 + base; \
  indices[ CNT ] = 7 + base; \
  indices[ CNT ] = 3 + base; \
  indices[ CNT ] = 2 + base; \
  indices[ CNT ] = 8 + base; \
  indices[ CNT ] = 12 + base; \
  indices[ CNT ] = 9 + base; \
  indices[ CNT ] = 9 + base; \
  indices[ CNT ] = 12 + base; \
  indices[ CNT ] = 10 + base; \
  indices[ CNT ] = 10 + base; \
  indices[ CNT ] = 12 + base; \
  indices[ CNT ] = 11 + base; \
  indices[ CNT ] = 11 + base; \
  indices[ CNT ] = 12 + base; \
  indices[ CNT ] = 8 + base; \
  indices[ CNT ] = 8 + base; \
  indices[ CNT ] = 9 + base; \
  indices[ CNT ] = 10 + base; \
  indices[ CNT ] = 10 + base; \
  indices[ CNT ] = 11 + base; \
  indices[ CNT ] = 8 + base;

  GAI( 0 );
  GAI( arrowVertexCount );
  GAI( arrowVertexCount * 2 );

  uint16_t indexBase = uint16_t( arrowVertexCount ) * 3;
  indices[ CNT ] = indexBase + 0;
  indices[ CNT ] = indexBase + 1;
  indices[ CNT ] = indexBase + 2;
  indices[ CNT ] = indexBase + 2;
  indices[ CNT ] = indexBase + 3;
  indices[ CNT ] = indexBase + 0;

  indexBase += planeVertexCount;
  indices[ CNT ] = indexBase + 0;
  indices[ CNT ] = indexBase + 1;
  indices[ CNT ] = indexBase + 2;
  indices[ CNT ] = indexBase + 2;
  indices[ CNT ] = indexBase + 3;
  indices[ CNT ] = indexBase + 0;

  indexBase += planeVertexCount;
  indices[ CNT ] = indexBase + 0;
  indices[ CNT ] = indexBase + 1;
  indices[ CNT ] = indexBase + 2;
  indices[ CNT ] = indexBase + 2;
  indices[ CNT ] = indexBase + 3;
  indices[ CNT ] = indexBase + 0;

  auto& device = RenderManager::GetInstance().GetDevice();

  vertexCount = _countof( vertices );
  indexCount  = _countof( indices  );

  vertexBuffer = CreateBufferFromData( vertices, vertexCount, ResourceType::VertexBuffer, device, commandList, L"GizmoVB" );
  indexBuffer  = CreateBufferFromData( indices,  indexCount,  ResourceType::IndexBuffer,  device, commandList, L"GizmoIB" );

  primitiveType = PrimitiveType::TriangleList;
}

void Gizmo::CreateRotateGeometry( CommandList& commandList )
{
  const int segCount = 32;

  int index = 0;

  const size_t circleVertexCount = segCount * 2;
  const size_t vertexCount       = circleVertexCount * 3;
  GizmoVertexFormat vertices[ vertexCount ];

  for ( int seg = 0; seg < segCount; seg++ )
  {
    float angle = XM_2PI * float( seg ) / segCount;
    float x     = sin( angle );
    float z     = cos( angle );
    vertices[ CNT ] = { { x * ci, 0, z * ci }, 0x0200FF00 };
    vertices[ CNT ] = { { x * co, 0, z * co }, 0x0200FF00 };
  }
  for ( int seg = 0; seg < segCount; seg++ )
  {
    float angle = XM_2PI * float( seg ) / segCount;
    float x = sin( angle );
    float y = cos( angle );
    vertices[ CNT ] = { { x * ci, y * ci, 0 }, 0x04FF0000 };
    vertices[ CNT ] = { { x * co, y * co, 0 }, 0x04FF0000 };
  }
  for ( int seg = 0; seg < segCount; seg++ )
  {
    float angle = XM_2PI * float( seg ) / segCount;
    float y = sin( angle );
    float z = cos( angle );
    vertices[ CNT ] = { { 0, y * ci, z * ci }, 0x010000FF };
    vertices[ CNT ] = { { 0, y * co, z * co }, 0x010000FF };
  }

  const size_t circleIndexCount = segCount * 2 + 3;
  const size_t indexCount       = circleIndexCount * 3;
  uint16_t indices[ indexCount ];

  index = 0;
  uint16_t vertexBase = 0;

  for ( int cix = 0; cix < 3; cix++ )
  {
    for ( int seg = 0; seg < segCount; seg++ )
    {
      indices[ CNT ] = vertexBase + seg * 2 + 0;
      indices[ CNT ] = vertexBase + seg * 2 + 1;
    }
    indices[ CNT ] = vertexBase;
    indices[ CNT ] = vertexBase + 1;
    indices[ CNT ] = 0xFFFF;

    vertexBase += circleVertexCount;
  }

  auto& device = RenderManager::GetInstance().GetDevice();

  this->vertexCount = _countof( vertices );
  this->indexCount  = _countof( indices  );

  vertexBuffer = CreateBufferFromData( vertices, vertexCount, ResourceType::VertexBuffer, device, commandList, L"GizmoVB" );
  indexBuffer  = CreateBufferFromData( indices,  indexCount,  ResourceType::IndexBuffer,  device, commandList, L"GizmoIB" );

  primitiveType = PrimitiveType::TriangleStrip;
}

void Gizmo::CreateScaleGeometry( CommandList& commandList )
{
  int index = 0;

  const size_t vertexCount = 4;
  GizmoVertexFormat vertices[ vertexCount ];

  vertices[ CNT ] = { { 0,   0,  0 }, 0x07FFFFFF };
  vertices[ CNT ] = { { sc,  0,  0 }, 0x07FFFFFF };
  vertices[ CNT ] = { { 0,  sc,  0 }, 0x07FFFFFF };
  vertices[ CNT ] = { { 0,   0, sc }, 0x07FFFFFF };

  const size_t indexCount = 12;
  uint16_t indices[ indexCount ];

  index = 0;

  indices[ CNT ] = 0;
  indices[ CNT ] = 2;
  indices[ CNT ] = 1;
  indices[ CNT ] = 0;
  indices[ CNT ] = 3;
  indices[ CNT ] = 2;
  indices[ CNT ] = 0;
  indices[ CNT ] = 1;
  indices[ CNT ] = 3;
  indices[ CNT ] = 1;
  indices[ CNT ] = 2;
  indices[ CNT ] = 3;

  auto& device = RenderManager::GetInstance().GetDevice();

  this->vertexCount = _countof( vertices );
  this->indexCount  = _countof( indices  );

  vertexBuffer = CreateBufferFromData( vertices, vertexCount, ResourceType::VertexBuffer, device, commandList, L"GizmoVB" );
  indexBuffer  = CreateBufferFromData( indices,  indexCount,  ResourceType::IndexBuffer,  device, commandList, L"GizmoIB" );
  
  primitiveType = PrimitiveType::TriangleList;
}

GizmoElement Gizmo::PickMove( FXMMATRIX xform, FXMVECTOR rayOrigin, FXMVECTOR rayDirection ) const
{
  XMFLOAT3 offset( XMVectorGetX( xform.r[ 3 ] ), XMVectorGetY( xform.r[ 3 ] ), XMVectorGetZ( xform.r[ 3 ] ) );
  BoundingBox ax, ay, az, pxy, pxz, pyz;
  XMFLOAT3 axc[ 2 ] = { XMFLOAT3( l0 + offset.x, -r1 + offset.y, -r1 + offset.z )
                      , XMFLOAT3( lt + offset.x,  r1 + offset.y,  r1 + offset.z ) };
  BoundingBox::CreateFromPoints( ax, 2, axc, sizeof XMFLOAT3 );

  XMFLOAT3 ayc[ 2 ] = { XMFLOAT3( -r1 + offset.x, l0 + offset.y, -r1 + offset.z )
                      , XMFLOAT3(  r1 + offset.x, lt + offset.y,  r1 + offset.z ) };
  BoundingBox::CreateFromPoints( ay, 2, ayc, sizeof XMFLOAT3 );

  XMFLOAT3 azc[ 2 ] = { XMFLOAT3( -r1 + offset.x, -r1 + offset.y, l0 + offset.z )
                      , XMFLOAT3(  r1 + offset.x,  r1 + offset.y, lt + offset.z ) };
  BoundingBox::CreateFromPoints( az, 2, azc, sizeof XMFLOAT3 );

  XMFLOAT3 pxyc[ 2 ] = { XMFLOAT3( r1 + offset.x, r1 + offset.y, -0.01f + offset.z )
                       , XMFLOAT3( ps + offset.x, ps + offset.y,  0.01f + offset.z ) };
  BoundingBox::CreateFromPoints( pxy, 2, pxyc, sizeof XMFLOAT3 );

  XMFLOAT3 pxzc[ 2 ] = { XMFLOAT3( r1 + offset.x, -0.01f + offset.y, r1 + offset.z )
                       , XMFLOAT3( ps + offset.x,  0.01f + offset.y, ps + offset.z ) };
  BoundingBox::CreateFromPoints( pxz, 2, pxzc, sizeof XMFLOAT3 );

  XMFLOAT3 pyzc[ 2 ] = { XMFLOAT3( -0.01f + offset.x, r1 + offset.y, r1 + offset.z )
                       , XMFLOAT3(  0.01f + offset.x, ps + offset.y, ps + offset.z ) };
  BoundingBox::CreateFromPoints( pyz, 2, pyzc, sizeof XMFLOAT3 );

  GizmoElement result = GizmoElement::None;
  float t = FLT_MAX;
  
  float axisT = FLT_MAX;
  if ( ax.Intersects( rayOrigin, rayDirection, axisT ) && axisT < t )
  {
    result = GizmoElement::XAxis;
    t = axisT;
  }
  if ( ay.Intersects( rayOrigin, rayDirection, axisT ) && axisT < t )
  {
    result = GizmoElement::YAxis;
    t = axisT;
  }
  if ( az.Intersects( rayOrigin, rayDirection, axisT ) && axisT < t )
  {
    result = GizmoElement::ZAxis;
    t = axisT;
  }
  if ( pxy.Intersects( rayOrigin, rayDirection, axisT ) && axisT < t )
  {
    result = GizmoElement::XYPlane;
    t = axisT;
  }
  if ( pxz.Intersects( rayOrigin, rayDirection, axisT ) && axisT < t )
  {
    result = GizmoElement::XZPlane;
    t = axisT;
  }
  if ( pyz.Intersects( rayOrigin, rayDirection, axisT ) && axisT < t )
  {
    result = GizmoElement::YZPlane;
    t = axisT;
  }

  return result;
}

GizmoElement Gizmo::PickRotate( FXMMATRIX xform, FXMVECTOR rayOrigin, FXMVECTOR rayDirection ) const
{
  auto pxz = XMPlaneFromPointNormal( xform.r[ 3 ], XMVectorSet( 0, 1, 0, 1 ) );
  auto pxy = XMPlaneFromPointNormal( xform.r[ 3 ], XMVectorSet( 0, 0, 1, 1 ) );
  auto pyz = XMPlaneFromPointNormal( xform.r[ 3 ], XMVectorSet( 1, 0, 0, 1 ) );

  auto ipxz = XMPlaneIntersectLine( pxz, rayOrigin, XMVectorAdd( rayOrigin, ( XMVectorScale( rayDirection, 100 ) ) ) );
  auto ipxy = XMPlaneIntersectLine( pxy, rayOrigin, XMVectorAdd( rayOrigin, ( XMVectorScale( rayDirection, 100 ) ) ) );
  auto ipyz = XMPlaneIntersectLine( pyz, rayOrigin, XMVectorAdd( rayOrigin, ( XMVectorScale( rayDirection, 100 ) ) ) );

  auto dpxz = XMVectorGetX( XMVector3Length( xform.r[ 3 ] - ipxz ) );
  auto dpxy = XMVectorGetX( XMVector3Length( xform.r[ 3 ] - ipxy ) );
  auto dpyz = XMVectorGetX( XMVector3Length( xform.r[ 3 ] - ipyz ) );

  if ( dpxz <= co && dpxz >= ci )
    return GizmoElement::YAxis;
  if ( dpxy <= co && dpxy >= ci )
    return GizmoElement::ZAxis;
  if ( dpyz <= co && dpyz >= ci )
    return GizmoElement::XAxis;

  return GizmoElement::None;
}

GizmoElement Gizmo::PickScale( FXMMATRIX xform, FXMVECTOR rayOrigin, FXMVECTOR rayDirection ) const
{
  XMVECTOR vertices[ 4 ] =
  {
    XMVectorAdd( XMVectorSet(  0,  0,  0, 1 ), xform.r[ 3 ] ),
    XMVectorAdd( XMVectorSet( sc,  0,  0, 1 ), xform.r[ 3 ] ),
    XMVectorAdd( XMVectorSet(  0, sc,  0, 1 ), xform.r[ 3 ] ),
    XMVectorAdd( XMVectorSet(  0,  0, sc, 1 ), xform.r[ 3 ] ),
  };

  float axisT = FLT_MAX;
  if ( TriangleTests::Intersects( rayOrigin, rayDirection, vertices[ 0 ], vertices[ 2 ], vertices[ 1 ], axisT )
    || TriangleTests::Intersects( rayOrigin, rayDirection, vertices[ 0 ], vertices[ 3 ], vertices[ 2 ], axisT ) 
    || TriangleTests::Intersects( rayOrigin, rayDirection, vertices[ 0 ], vertices[ 1 ], vertices[ 3 ], axisT )
    || TriangleTests::Intersects( rayOrigin, rayDirection, vertices[ 1 ], vertices[ 2 ], vertices[ 3 ], axisT ) )
    return GizmoElement::AllAxes;

  return GizmoElement::None;
}

XMVECTOR Gizmo::CalcManipulate( GizmoElement pickedGizmoElement, const Camera& camera, XMINT2 fromScreen, XMINT2 toScreen, FXMMATRIX gizmoTransform )
{
  switch ( type )
  {
  case GizmoType::Move:
    return CalcMove( pickedGizmoElement, camera, fromScreen, toScreen, gizmoTransform );
  case GizmoType::Scale:
    return CalcScale( pickedGizmoElement, camera, fromScreen, toScreen, gizmoTransform );
  case GizmoType::Rotate:
    return CalcRotate( pickedGizmoElement, camera, fromScreen, toScreen, gizmoTransform );
  default:
    assert( false );
    return g_XMZero;
  }
}

XMVECTOR Gizmo::CalcMove( GizmoElement pickedGizmoElement, const Camera& camera, XMINT2 fromScreen, XMINT2 toScreen, FXMMATRIX gizmoTransform )
{
  XMVECTOR planeHelper1, planeHelper2, axisMask;
  switch ( pickedGizmoElement )
  {
  case GizmoElement::XAxis:
    planeHelper1 = XMVectorSet( 1, 0, 0, 1 );
    planeHelper2 = camera.GetUp();
    axisMask     = XMVectorSet( 1, 0, 0, 1 );
    break;
  case GizmoElement::YAxis:
    planeHelper1 = XMVectorSet( 0, 1, 0, 1 );
    planeHelper2 = camera.GetRight();
    axisMask     = XMVectorSet( 0, 1, 0, 1 );
    break;
  case GizmoElement::ZAxis:
    planeHelper1 = XMVectorSet( 0, 0, 1, 1 );
    planeHelper2 = camera.GetUp();
    axisMask     = XMVectorSet( 0, 0, 1, 1 );
    break;
  case GizmoElement::XYPlane:
    planeHelper1 = XMVectorSet( 1, 0, 0, 1 );
    planeHelper2 = XMVectorSet( 0, 1, 0, 1 );
    axisMask     = XMVectorSet( 1, 1, 0, 1 );
    break;
  case GizmoElement::XZPlane:
    planeHelper1 = XMVectorSet( 1, 0, 0, 1 );
    planeHelper2 = XMVectorSet( 0, 0, 1, 1 );
    axisMask     = XMVectorSet( 1, 0, 1, 1 );
    break;
  case GizmoElement::YZPlane:
    planeHelper1 = XMVectorSet( 0, 1, 0, 1 );
    planeHelper2 = XMVectorSet( 0, 0, 1, 1 );
    axisMask     = XMVectorSet( 0, 1, 1, 1 );
    break;
  default:
    assert( false );
    break;
  }

  auto fromRay = camera.GetRayFromScreen( fromScreen.x, fromScreen.y );
  auto toRay   = camera.GetRayFromScreen(   toScreen.x,   toScreen.y );
  
  auto fromStart = XMLoadFloat3( &fromRay.first );
  auto fromEnd   = fromStart + XMLoadFloat3( &fromRay.second ) * 1000;

  auto toStart = XMLoadFloat3( &toRay.first );
  auto toEnd   = toStart + XMLoadFloat3( &toRay.second ) * 1000;

  auto entityPos = gizmoTransform.r[ 3 ];
  auto toolPlane = XMPlaneFromPoints( entityPos, entityPos + planeHelper1, entityPos + planeHelper2 );
  auto startLoc  = XMPlaneIntersectLine( toolPlane, fromStart, fromEnd );
  auto endLoc    = XMPlaneIntersectLine( toolPlane, toStart, toEnd );
  auto dv        = endLoc - startLoc;
  auto d         = XMVector3Length( dv );

  dv = XMVector3Normalize( dv * axisMask );
  dv = dv * d;

  return dv;
}

XMVECTOR Gizmo::CalcRotate( GizmoElement pickedGizmoElement, const Camera& camera, XMINT2 fromScreen, XMINT2 toScreen, FXMMATRIX gizmoTransform )
{
  XMVECTOR toolPlaneNormal;
  XMVECTOR axisMask;
  switch ( pickedGizmoElement )
  {
  case GizmoElement::XAxis:
    toolPlaneNormal = XMVectorSet( 1, 0, 0, 1 );
    axisMask        = XMVectorSet( 1, 0, 0, 1 );
    break;
  case GizmoElement::YAxis:
    toolPlaneNormal = XMVectorSet( 0, 1, 0, 1 );
    axisMask        = XMVectorSet( 0, 1, 0, 1 );
    break;
  case GizmoElement::ZAxis:
    toolPlaneNormal = XMVectorSet( 0, 0, 1, 1 );
    axisMask        = XMVectorSet( 0, 0, 1, 1 );
    break;
  default:
    assert( false );
    break;
  }
  auto entityPos   = gizmoTransform.r[ 3 ];
  auto toolPlane   = XMPlaneFromPointNormal( entityPos, toolPlaneNormal );
  auto fromRay     = camera.GetRayFromScreen( fromScreen.x, fromScreen.y );
  auto toRay       = camera.GetRayFromScreen(   toScreen.x,   toScreen.y );
  auto fromStart   = XMLoadFloat3( &fromRay.first );
  auto fromEnd     = fromStart + XMLoadFloat3( &fromRay.second ) * 1000;
  auto toStart     = XMLoadFloat3( &toRay.first );
  auto toEnd       = toStart + XMLoadFloat3( &toRay.second ) * 1000;
  auto cameraPos   = camera.GetPosition();
  auto startLoc    = XMPlaneIntersectLine( toolPlane, fromStart, fromEnd );
  auto endLoc      = XMPlaneIntersectLine( toolPlane, toStart, toEnd );
  auto toStartLoc  = startLoc - entityPos;
  auto toEndLoc    = endLoc - entityPos;
  auto da          = XMVectorGetX( XMVector3AngleBetweenVectors( toStartLoc, toEndLoc ) );
  auto angleNormal = XMVector3Cross( toStartLoc, toEndLoc );
  auto angleSign   = XMVectorGetX( XMVector3Dot( angleNormal, toolPlaneNormal ) );
  
  angleSign = ( angleSign < 0 ) ? -1.0f : 1.0f;

  return axisMask * XMConvertToDegrees( da * angleSign );
}

XMVECTOR Gizmo::CalcScale( GizmoElement pickedGizmoElement, const Camera& camera, XMINT2 fromScreen, XMINT2 toScreen, FXMMATRIX gizmoTransform )
{
  float dx = float( toScreen.x - fromScreen.x );
  return g_XMOne * ( dx / 100 );
}
