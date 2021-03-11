#include "LightEntity.h"
#include "../External/tinyxml2/tinyxml2.h"

std::unique_ptr< LightEntity > LightEntity::CreateDeserialize( tinyxml2::XMLNode& node )
{
  auto entity = std::make_unique< LightEntity >();
  entity.get()->Deserialize( node );
  return entity;
}

LightEntity::LightEntity()
{
}

void LightEntity::Serialize( uint32_t id, tinyxml2::XMLNode& node )
{
  using namespace tinyxml2;

  auto entityElement = static_cast< XMLElement* >( node.InsertEndChild( node.GetDocument()->NewElement( "LightEntity" ) ) );

  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "Type" ) ) )->SetAttribute( "number", (int)type );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "Intensity" ) ) )->SetAttribute( "value", intensity );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "SourceRadius" ) ) )->SetAttribute( "value", sourceRadius );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "AttenuationStart" ) ) )->SetAttribute( "value", attenuationStart );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "AttenuationEnd" ) ) )->SetAttribute( "value", attenuationEnd );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "AttenuationFalloff" ) ) )->SetAttribute( "value", attenuationFalloff );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "Theta" ) ) )->SetAttribute( "value", theta );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "Phi" ) ) )->SetAttribute( "value", phi );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "ConeFalloff" ) ) )->SetAttribute( "value", coneFalloff );
  static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "CastShadow" ) ) )->SetAttribute( "value", castShadow );

  auto colorElement = static_cast< XMLElement* >( entityElement->InsertEndChild( node.GetDocument()->NewElement( "Color" ) ) );
  colorElement->SetAttribute( "r", color.r );
  colorElement->SetAttribute( "g", color.g );
  colorElement->SetAttribute( "b", color.b );

  Entity::Serialize( id, *entityElement );
}

void LightEntity::Deserialize( tinyxml2::XMLNode& node )
{
  Entity::Deserialize( node );

  if ( auto prop = node.FirstChildElement( "Color" ) )
  {
    prop->QueryFloatAttribute( "r", &color.r );
    prop->QueryFloatAttribute( "g", &color.g );
    prop->QueryFloatAttribute( "b", &color.b );
  }

  if ( auto prop = node.FirstChildElement( "Type" ) )
  {
    int val;
    prop->QueryIntAttribute( "number", &val );
    type = LightTypeCB( val );
  }

  if ( auto prop = node.FirstChildElement( "Intensity" ) )
    prop->QueryFloatAttribute( "value", &intensity );

  if ( auto prop = node.FirstChildElement( "SourceRadius" ) )
    prop->QueryFloatAttribute( "value", &sourceRadius );

  if ( auto prop = node.FirstChildElement( "AttenuationStart" ) )
    prop->QueryFloatAttribute( "value", &attenuationStart );

  if ( auto prop = node.FirstChildElement( "AttenuationEnd" ) )
    prop->QueryFloatAttribute( "value", &attenuationEnd );

  if ( auto prop = node.FirstChildElement( "AttenuationFalloff" ) )
    prop->QueryFloatAttribute( "value", &attenuationFalloff );

  if ( auto prop = node.FirstChildElement( "Theta" ) )
    prop->QueryFloatAttribute( "value", &theta );

  if ( auto prop = node.FirstChildElement( "Phi" ) )
    prop->QueryFloatAttribute( "value", &phi );

  if ( auto prop = node.FirstChildElement( "ConeFalloff" ) )
    prop->QueryFloatAttribute( "value", &coneFalloff );

  if ( auto prop = node.FirstChildElement( "CastShadow" ) )
    prop->QueryBoolAttribute( "value", &castShadow );
}

float LightEntity::Pick( FXMVECTOR rayStart, FXMVECTOR rayDir ) const
{
  float d;
  BoundingSphere bounding( position, 0.3f );
  if ( bounding.Intersects( rayStart, rayDir, d ) )
    return d;

  return -1;
}

std::unique_ptr< Entity > LightEntity::Duplicate( CommandList& commandList )
{
  auto light = std::make_unique< LightEntity >();

  light->type               = type;
  light->color              = color;
  light->intensity          = intensity;
  light->sourceRadius       = sourceRadius;
  light->attenuationStart   = attenuationStart;
  light->attenuationEnd     = attenuationEnd;
  light->attenuationFalloff = attenuationFalloff;
  light->theta              = theta;
  light->phi                = phi;
  light->coneFalloff        = coneFalloff;
  light->castShadow         = castShadow;
  
  light->position = position;
  light->rotation = rotation;
  light->scale    = scale;

  light->worldTransform = worldTransform;

  light->TransformChanged();

  return light;
}

LightTypeCB LightEntity::GetType() const
{
  return type;
}

Color LightEntity::GetColor() const
{
  return color;
}

float LightEntity::GetIntensity() const
{
  return intensity;
}

float LightEntity::GetSourceRadius() const
{
  return sourceRadius;
}

float LightEntity::GetAttenuationStart() const
{
  return attenuationStart;
}

float LightEntity::GetAttenuationEnd() const
{
  return attenuationEnd;
}

float LightEntity::GetAttenuationFalloff() const
{
  return attenuationFalloff;
}

float LightEntity::GetTheta() const
{
  return theta;
}

float LightEntity::GetPhi() const
{
  return phi;
}

float LightEntity::GetConeFalloff() const
{
  return coneFalloff;
}

bool LightEntity::GetCastShadow() const
{
  return castShadow;
}

void LightEntity::SetType( LightTypeCB v )
{
  type = v;
}

void LightEntity::SetColor( const Color& v )
{
  color = v;
}

void LightEntity::SetIntensity( float v )
{
  intensity = v;
}

void LightEntity::SetSourceRadius( float v )
{
  sourceRadius = v;
}

void LightEntity::SetAttenuationStart( float v )
{
  attenuationStart = v;
}

void LightEntity::SetAttenuationEnd( float v )
{
  attenuationEnd = v;
}

void LightEntity::SetAttenuationFalloff( float v )
{
  attenuationFalloff = v;
}

void LightEntity::SetTheta( float v )
{
  theta = v;
}

void LightEntity::SetPhi( float v )
{
  phi = v;
}

void LightEntity::SetConeFalloff( float v )
{
  coneFalloff = v;
}

void LightEntity::SetCastShadow( bool v )
{
  castShadow = v;
}

LightCB LightEntity::GetLight() const
{
  LightCB light;
  light.origin             = XMFLOAT4( position.x, position.y, position.z, 1 );
  light.direction          = XMFLOAT4( worldTransform._13, worldTransform._23, worldTransform._33, 1 );
  light.color              = XMFLOAT4( color.r * intensity, color.g * intensity, color.b * intensity, 1 );
  light.sourceRadius       = sourceRadius;
  light.attenuationStart   = attenuationStart;
  light.attenuationEnd     = attenuationEnd;
  light.attenuationFalloff = attenuationFalloff;
  light.theta              = std::cos( XMConvertToRadians( theta ) / 2 );
  light.phi                = std::cos( XMConvertToRadians( phi ) / 2 );
  light.coneFalloff        = coneFalloff;
  light.castShadow         = castShadow ? 1 : 0;
  light.type               = type;
  return light;
}

void LightEntity::TransformChanged()
{
}
