#include "Entity.h"
#include "../External/tinyxml2/tinyxml2.h"

Entity::~Entity()
{
}

XMVECTOR Entity::GetPosition() const
{
  return XMLoadFloat3( &position );
}

XMVECTOR Entity::GetRotation() const
{
  return XMLoadFloat3( &rotation );
}

XMVECTOR Entity::GetScale() const
{
  return XMLoadFloat3( &scale );
}

XMMATRIX Entity::GetWorldTransform() const
{
  return XMLoadFloat4x4( &worldTransform );
}

void Entity::SetPosition( FXMVECTOR v )
{
  XMStoreFloat3( &position, v );
  RecalcWorldTransform();
}

void Entity::SetRotation( FXMVECTOR v )
{
  XMStoreFloat3( &rotation, v );
  RecalcWorldTransform();
}

void Entity::SetScale( FXMVECTOR v )
{
  XMStoreFloat3( &scale, v );
  RecalcWorldTransform();
}

void Entity::Serialize( uint32_t id, tinyxml2::XMLNode& node )
{
  using namespace tinyxml2;

  static_cast< XMLElement& >( node ).SetAttribute( "id", id );

  auto positionElement = static_cast< XMLElement* >( node.InsertEndChild( node.GetDocument()->NewElement( "Position" ) ) );
  positionElement->SetAttribute( "x", position.x );
  positionElement->SetAttribute( "y", position.y );
  positionElement->SetAttribute( "z", position.z );

  auto rotationElement = static_cast< XMLElement* >( node.InsertEndChild( node.GetDocument()->NewElement( "Rotation" ) ) );
  rotationElement->SetAttribute( "x", rotation.x );
  rotationElement->SetAttribute( "y", rotation.y );
  rotationElement->SetAttribute( "z", rotation.z );

  auto scaleElement = static_cast< XMLElement* >( node.InsertEndChild( node.GetDocument()->NewElement( "Scale" ) ) );
  scaleElement->SetAttribute( "x", scale.x );
  scaleElement->SetAttribute( "y", scale.y );
  scaleElement->SetAttribute( "z", scale.z );
}

void Entity::Deserialize( tinyxml2::XMLNode& node )
{
  using namespace tinyxml2;

  auto positionElement = node.FirstChildElement( "Position" );
  positionElement->QueryFloatAttribute( "x", &position.x );
  positionElement->QueryFloatAttribute( "y", &position.y );
  positionElement->QueryFloatAttribute( "z", &position.z );

  auto rotationElement = node.FirstChildElement( "Rotation" );
  rotationElement->QueryFloatAttribute( "x", &rotation.x );
  rotationElement->QueryFloatAttribute( "y", &rotation.y );
  rotationElement->QueryFloatAttribute( "z", &rotation.z );

  auto scaleElement = node.FirstChildElement( "Scale" );
  scaleElement->QueryFloatAttribute( "x", &scale.x );
  scaleElement->QueryFloatAttribute( "y", &scale.y );
  scaleElement->QueryFloatAttribute( "z", &scale.z );

  RecalcWorldTransform();
}

void Entity::RecalcWorldTransform()
{
  auto xmRotation    = XMQuaternionRotationRollPitchYaw( XMConvertToRadians( rotation.x ), XMConvertToRadians( rotation.y ), XMConvertToRadians( rotation.z ) );
  auto xmScaling     = XMLoadFloat3( &scale );
  auto xmTranslation = XMLoadFloat3( &position );
  XMStoreFloat4x4( &worldTransform, XMMatrixAffineTransformation( xmScaling, XMVectorSet( 0, 0, 0, 0 ), xmRotation, xmTranslation ) );

  TransformChanged();
}
