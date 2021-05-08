#pragma once

#include "Camera.h"
#include "Common/Signal.h"
#include "Common/Finally.h"

class DirectionalCamera : public Camera
{
public:
  DirectionalCamera( float fovY );
  ~DirectionalCamera();

  virtual BoundingFrustum GetInfiniteFrustum() override;

  void SetDepthRange( float nearZ, float farZ ) override;

  std::pair< XMFLOAT3, XMFLOAT3 > GetRayFromScreen( int x, int y ) const override;

  XMVECTOR GetPosition() const override;
  XMVECTOR GetDirection() const override;
  XMVECTOR GetRight() const override;
  XMVECTOR GetUp() const override;

  XMMATRIX GetViewTransform() const override;
  XMMATRIX GetProjectionTransform( bool includeJitter ) const override;
  XMMATRIX GetViewProjectionTransform( bool includeJitter ) const override;

  float GetNearDepth() const override;
  float GetFarDepth() const override;

  void SetJitter( XMFLOAT2 jitter ) override;

  float GetFovY() const;
  float GetAspect() const;

  void SetFovY( float fovY );
  void SetSize( float width, float height );

  void MoveBy( CXMVECTOR position );
  void MoveTo( CXMVECTOR position );
  void RotateX( float angle );
  void RotateY( float angle );
  void RotateTo( CXMVECTOR angles );

private:
  void Recalculate() const;

  XMFLOAT3A position;
  XMFLOAT3A direction;
  XMFLOAT3A right;
  XMFLOAT3A up;

  float fovY;
  float width;
  float height;

  float nearZ = 0.1f;
  float farZ  = 100.0f;

  XMFLOAT2 jitter = XMFLOAT2( 0, 0 );

  mutable XMFLOAT4X4A viewTransform;
  mutable XMFLOAT4X4A projectionTransform;
  mutable XMFLOAT4X4A vpTransform;

  mutable XMFLOAT4X4A projectionTransformNoJitter;
  mutable XMFLOAT4X4A vpTransformNoJitter;

  mutable bool dirty = true;

  float moveX    = 0;
  float moveY    = 0;
  float moveZ    = 0;
  bool  rotating = false;
};
