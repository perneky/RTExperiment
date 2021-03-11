#include "CameraEntity.h"
#include "Render/DirectionalCamera.h"
#include "../External/tinyxml2/tinyxml2.h"

std::unique_ptr< CameraEntity > CameraEntity::CreateDeserialize( tinyxml2::XMLNode& node )
{
  auto entity = std::make_unique< CameraEntity >();
  entity->Deserialize( node );
  return entity;
}

CameraEntity::CameraEntity()
{
  camera = std::make_unique< DirectionalCamera >( fovY );
}

void CameraEntity::Serialize( uint32_t id, tinyxml2::XMLNode& node )
{
  using namespace tinyxml2;

  auto entityElement = static_cast< XMLElement* >( node.InsertEndChild( node.GetDocument()->NewElement( "CameraEntity" ) ) );

  Entity::Serialize( id, *entityElement );
}

float CameraEntity::GetFovY() const
{
  return fovY;
}

void CameraEntity::SetFovY( float v )
{
  fovY = v;
  camera->SetFovY( v );
}

void CameraEntity::SetSize( float width, float height )
{
  camera->SetSize( width, height );
}

Camera& CameraEntity::GetCamera()
{
  return *camera;
}

const Camera& CameraEntity::GetCamera() const
{
  return *camera;
}

void CameraEntity::TransformChanged()
{
  camera->MoveTo( XMLoadFloat3( &position ) );
  camera->RotateTo( XMLoadFloat3( &rotation ) );
}

std::unique_ptr< Entity > CameraEntity::Duplicate( CommandList& commandList )
{
  auto camera = std::make_unique< CameraEntity >();

  camera->fovY = fovY;
  
  camera->position = position;
  camera->rotation = rotation;
  camera->scale    = scale;

  camera->worldTransform = worldTransform;

  camera->TransformChanged();

  return camera;
}
