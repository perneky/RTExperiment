#pragma once

#include "Entity.h"
#include "Common/Color.h"
#include "Render/ShaderStructuresNative.h"

class LightEntity : public Entity
{
public:
  static std::unique_ptr< LightEntity > CreateDeserialize( tinyxml2::XMLNode& node );

  LightEntity();

  void Serialize( uint32_t id, tinyxml2::XMLNode& node ) override;
  void Deserialize( tinyxml2::XMLNode& node ) override;

  float Pick( FXMVECTOR rayStart, FXMVECTOR rayDir ) const override;

  std::unique_ptr< Entity > Duplicate( CommandList& commandList ) override;

  LightTypeCB GetType() const;
  Color       GetColor() const;
  float       GetIntensity() const;
  float       GetSourceRadius() const;
  float       GetAttenuationStart() const;
  float       GetAttenuationEnd() const;
  float       GetAttenuationFalloff() const;
  float       GetTheta() const;
  float       GetPhi() const;
  float       GetConeFalloff() const;
  bool        GetCastShadow() const;

  void SetType( LightTypeCB v );
  void SetColor( const Color& v );
  void SetIntensity( float v );
  void SetSourceRadius( float v );
  void SetAttenuationStart( float v );
  void SetAttenuationEnd( float v );
  void SetAttenuationFalloff( float v );
  void SetTheta( float v );
  void SetPhi( float v );
  void SetConeFalloff( float v );
  void SetCastShadow( bool v );

  LightCB GetLight() const;

protected:
  void TransformChanged() override;

  LightTypeCB type               = LightTypeCB::Point;
  Color       color              = Color( 1, 1, 1 );
  float       intensity          = 1;
  float       sourceRadius       = 0.1f;
  float       attenuationStart   = 0.2f;
  float       attenuationEnd     = 10.0;
  float       attenuationFalloff = 2;
  float       theta              = 30;
  float       phi                = 60;
  float       coneFalloff        = 2;
  bool        castShadow         = true;
};