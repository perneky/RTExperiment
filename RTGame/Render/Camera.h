#pragma once

struct Camera
{
  virtual ~Camera() = default;

  virtual BoundingFrustum GetInfiniteFrustum() = 0;

  virtual void SetDepthRange( float nearZ, float farZ ) = 0;

  virtual std::pair< XMFLOAT3, XMFLOAT3 > GetRayFromScreen( int x, int y ) const = 0;

  virtual XMVECTOR GetPosition() const = 0;
  virtual XMVECTOR GetDirection() const = 0;
  virtual XMVECTOR GetRight() const = 0;
  virtual XMVECTOR GetUp() const = 0;

  virtual XMMATRIX GetViewTransform() const = 0;
  virtual XMMATRIX GetProjectionTransform( bool includeJitter ) const = 0;
  virtual XMMATRIX GetViewProjectionTransform( bool includeJitter ) const = 0;

  virtual void SetJitter( XMFLOAT2 jitter ) = 0;
};
