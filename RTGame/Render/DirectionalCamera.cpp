#include "DirectionalCamera.h"

DirectionalCamera::DirectionalCamera( float fovY )
  : fovY( fovY )
  , width( 1 )
  , height( 1 )
{
  position  = XMFLOAT3A( 0, 0, 0 );
  direction = XMFLOAT3A( 0, 0, 1 );
  right     = XMFLOAT3A( 1, 0, 0 );
  up        = XMFLOAT3A( 0, 1, 0 );
}

DirectionalCamera::~DirectionalCamera()
{
}

BoundingFrustum DirectionalCamera::GetInfiniteFrustum()
{
  Recalculate();

  auto viewMatrix = XMLoadFloat4x4( &viewTransform );
  auto projMatrix = XMMatrixPerspectiveFovLH( XMConvertToRadians( fovY ), width / height, 0.01f, 10000 );
  auto vpMatrix = XMMatrixMultiply( viewMatrix, projMatrix );

  BoundingFrustum result;
  BoundingFrustum::CreateFromMatrix( result, vpMatrix);
  return result;
}

std::pair< XMFLOAT3, XMFLOAT3 > DirectionalCamera::GetRayFromScreen( int x, int y ) const
{
  Recalculate();

  auto nearPoint = XMVectorSet( float( x ), float( y ), 0, 1 );
  auto farPoint  = XMVectorSet( float( x ), float( y ), 1, 1 );
  auto nearWorld = XMVector3Unproject( nearPoint, 0, 0, width, height, 0, 1, XMLoadFloat4x4( &projectionTransform ), XMLoadFloat4x4( &viewTransform ), XMMatrixIdentity() );
  auto farWorld  = XMVector3Unproject( farPoint,  0, 0, width, height, 0, 1, XMLoadFloat4x4( &projectionTransform ), XMLoadFloat4x4( &viewTransform ), XMMatrixIdentity() );
  auto dir       = XMVector3Normalize( XMVectorSubtract( farWorld, nearWorld ) );

  XMFLOAT3 rayStart;
  XMFLOAT3 rayDir;

  XMStoreFloat3( &rayStart, nearWorld );
  XMStoreFloat3( &rayDir,   dir       );

  return { rayStart, rayDir };
}

void DirectionalCamera::Recalculate() const
{
  if ( !dirty )
    return;

  auto viewMatrix         = XMMatrixLookToLH( XMLoadFloat3A( &position ), XMLoadFloat3A( &direction ), XMLoadFloat3A( &up ) );
  auto projMatrix         = XMMatrixPerspectiveFovLH( XMConvertToRadians( fovY ), width / height, nearZ, farZ );
  auto projMatrixNoJitter = projMatrix;

  projMatrix.r[ 2 ] += XMVectorSet( jitter.x, jitter.y, 0, 0 );

  auto vpMatrix         = XMMatrixMultiply( viewMatrix, projMatrix );
  auto vpMatrixNoJitter = XMMatrixMultiply( viewMatrix, projMatrixNoJitter );

  XMStoreFloat4x4A( &viewTransform,               viewMatrix );
  XMStoreFloat4x4A( &projectionTransform,         projMatrix );
  XMStoreFloat4x4A( &vpTransform,                 vpMatrix );
  XMStoreFloat4x4A( &projectionTransformNoJitter, projMatrixNoJitter );
  XMStoreFloat4x4A( &vpTransformNoJitter,         vpMatrixNoJitter );

  dirty = false;
}

XMVECTOR DirectionalCamera::GetPosition() const
{
  return XMLoadFloat3A( &position );
}

XMVECTOR DirectionalCamera::GetDirection() const
{
  return XMLoadFloat3A( &direction );
}

XMVECTOR DirectionalCamera::GetRight() const
{
  return XMLoadFloat3A( &right );
}

XMVECTOR DirectionalCamera::GetUp() const
{
  return XMLoadFloat3A( &up );
}

XMMATRIX DirectionalCamera::GetViewTransform() const
{
  Recalculate();
  return XMLoadFloat4x4A( &viewTransform );
}

XMMATRIX DirectionalCamera::GetProjectionTransform( bool includeJitter ) const
{
  Recalculate();
  return XMLoadFloat4x4A( includeJitter ? &projectionTransform : &projectionTransformNoJitter );
}

XMMATRIX DirectionalCamera::GetViewProjectionTransform( bool includeJitter ) const
{
  Recalculate();
  return XMLoadFloat4x4A( includeJitter ? &vpTransform : &vpTransformNoJitter );
}

void DirectionalCamera::SetJitter( XMFLOAT2 jitter )
{
  this->jitter = jitter;
  dirty = true;
}

float DirectionalCamera::GetFovY() const
{
  return fovY;
}

float DirectionalCamera::GetAspect() const
{
  return width / height;
}

void DirectionalCamera::SetFovY( float fovY )
{
  this->fovY = fovY;
  dirty = true;
}

void DirectionalCamera::SetSize( float width, float height )
{
  this->width  = width;
  this->height = height;
  dirty = true;
}

void DirectionalCamera::SetDepthRange( float nearZ, float farZ )
{
  this->nearZ = nearZ;
  this->farZ  = farZ;
  dirty = true;
}

void DirectionalCamera::MoveBy( CXMVECTOR position )
{
  XMStoreFloat3A( &this->position, XMVectorAdd( XMLoadFloat3A( &this->position ), position ) );
  dirty = true;
}

void DirectionalCamera::MoveTo( CXMVECTOR position )
{
  XMStoreFloat3A( &this->position, position );
  dirty = true;
}

void DirectionalCamera::RotateX( float angle )
{
  auto xmDirection = XMLoadFloat3A( &direction );
  auto xmRight     = XMLoadFloat3A( &right );
  auto xmUp        = XMLoadFloat3A( &up );
  auto xmRotate    = XMMatrixRotationAxis( xmRight, XMConvertToRadians( angle ) );
  XMStoreFloat3A( &direction, XMVector3TransformNormal( xmDirection, xmRotate ) );
  XMStoreFloat3A( &right,     XMVector3TransformNormal( xmRight,     xmRotate ) );
  XMStoreFloat3A( &up,        XMVector3TransformNormal( xmUp,        xmRotate ) );
  dirty = true;
}

void DirectionalCamera::RotateY( float angle )
{
  auto xmDirection = XMLoadFloat3A( &direction );
  auto xmRight     = XMLoadFloat3A( &right );
  auto xmUp        = XMLoadFloat3A( &up );
  auto xmRotate    = XMMatrixRotationY( XMConvertToRadians( angle ) );
  XMStoreFloat3A( &direction, XMVector3TransformNormal( xmDirection, xmRotate ) );
  XMStoreFloat3A( &right,     XMVector3TransformNormal( xmRight,     xmRotate ) );
  XMStoreFloat3A( &up,        XMVector3TransformNormal( xmUp,        xmRotate ) );
  dirty = true;
}

void DirectionalCamera::RotateTo( CXMVECTOR angles )
{
  auto xmDirection = XMVectorSet( 0, 0, 1, 1 );
  auto xmRight     = XMVectorSet( 1, 0, 0, 1 );
  auto xmUp        = XMVectorSet( 0, 1, 0, 1 );
  auto xmRotate    = XMMatrixRotationRollPitchYaw( XMConvertToRadians( XMVectorGetX( angles ) ), XMConvertToRadians( XMVectorGetY( angles ) ), XMConvertToRadians( XMVectorGetZ( angles ) ) );
  XMStoreFloat3A( &direction, XMVector3TransformNormal( xmDirection, xmRotate ) );
  XMStoreFloat3A( &right,     XMVector3TransformNormal( xmRight,     xmRotate ) );
  XMStoreFloat3A( &up,        XMVector3TransformNormal( xmUp,        xmRotate ) );
  dirty = true;
}
