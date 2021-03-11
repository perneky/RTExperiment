#pragma once

#include "Entity.h"

class DirectionalCamera;
struct Camera;
struct Window;

class CameraEntity : public Entity
{
public:
  static std::unique_ptr< CameraEntity > CreateDeserialize( tinyxml2::XMLNode& node );

  CameraEntity();

  void Serialize( uint32_t id, tinyxml2::XMLNode& node ) override;

  std::unique_ptr< Entity > Duplicate( CommandList& commandList ) override;

  float GetFovY() const;
  
  void SetFovY( float v );
  void SetSize( float width, float height );

  Camera& GetCamera();
  const Camera& GetCamera() const;

protected:
  void TransformChanged() override;

  std::unique_ptr< DirectionalCamera > camera;

  float fovY = 90;
};