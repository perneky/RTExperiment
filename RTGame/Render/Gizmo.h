#pragma once

struct Resource;
struct CommandList;
struct Camera;
enum class PrimitiveType;

enum class GizmoType
{
  None,
  Move,
  Rotate,
  Scale,
};

enum GizmoElement
{
  None    = 0,
  XAxis   = 0x01,
  YAxis   = 0x02,
  ZAxis   = 0x04,
  XYPlane = XAxis | YAxis,
  XZPlane = XAxis | ZAxis,
  YZPlane = YAxis | ZAxis,
  AllAxes = XAxis | YAxis | ZAxis,
};

class Gizmo
{
public:
  Gizmo( CommandList& commandList, GizmoType type );
  ~Gizmo();

  void TearDown( CommandList& commandList );

  GizmoType GetType() const;

  void Render( CommandList& commandList, FXMMATRIX wvpTransform );

  GizmoElement Pick( FXMMATRIX xform, FXMVECTOR rayOrigin, FXMVECTOR rayDirection ) const;

  XMVECTOR CalcManipulate( GizmoElement pickedGizmoElement, const Camera& camera, XMINT2 fromScreen, XMINT2 toScreen, FXMMATRIX gizmoTransform );

private:
  void CreateMoveGeometry  ( CommandList& commandList );
  void CreateRotateGeometry( CommandList& commandList );
  void CreateScaleGeometry ( CommandList& commandList );

  GizmoElement PickMove  ( FXMMATRIX xform, FXMVECTOR rayOrigin, FXMVECTOR rayDirection ) const;
  GizmoElement PickRotate( FXMMATRIX xform, FXMVECTOR rayOrigin, FXMVECTOR rayDirection ) const;
  GizmoElement PickScale ( FXMMATRIX xform, FXMVECTOR rayOrigin, FXMVECTOR rayDirection ) const;

  XMVECTOR CalcMove  ( GizmoElement pickedGizmoElement, const Camera& camera, XMINT2 fromScreen, XMINT2 toScreen, FXMMATRIX gizmoTransform );
  XMVECTOR CalcRotate( GizmoElement pickedGizmoElement, const Camera& camera, XMINT2 fromScreen, XMINT2 toScreen, FXMMATRIX gizmoTransform );
  XMVECTOR CalcScale ( GizmoElement pickedGizmoElement, const Camera& camera, XMINT2 fromScreen, XMINT2 toScreen, FXMMATRIX gizmoTransform );

  GizmoType type;

  std::unique_ptr< Resource > vertexBuffer;
  std::unique_ptr< Resource > indexBuffer;
  std::unique_ptr< Resource > constantBuffer;

  int vertexCount;
  int indexCount;

  PrimitiveType primitiveType;
};