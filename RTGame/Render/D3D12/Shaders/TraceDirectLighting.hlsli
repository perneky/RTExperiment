#pragma once

#include "PBRUtils.hlsli"

struct LightCalcData
{
  float3 toLight;
  float3 lightCenter;
  float3 lightDirection;
  float  lightRadius;
  float  attenuationStart;
  float  attenuationEnd;
  float  attenuationFalloff;
  float  theta;
  float  phi;
  float  coneFalloff;
  bool   castShadow;
  bool   hasAttenuation;
  bool   isCone;
};

float3 CastToPoint( float3 worldPosition, float3 pointCoord, float minT )
{
  float3 origin  = worldPosition;
  float  maxT    = length( pointCoord - worldPosition );
  float3 toPoint = normalize( pointCoord - worldPosition );

  bool   ended = false;
  float3 color = 1;

  while ( !ended && any( color > 0 ) )
  {
    HitGeometry hitGeom = TraceRay( origin, toPoint, minT, maxT );

    [branch]
    if ( hitGeom.t >= 0 )
    {
      [branch]
      if ( IsOneBitAlphaMaterial( hitGeom.materialIndex ) )
      {
        float alpha = SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord ).a;
        [branch]
        if ( !ShouldBeDiscared( alpha ) )
        {
          color = 0;
          ended = true;
        }
      }
      else if ( IsTranslucentMaterial( hitGeom.materialIndex ) )
      {
        bool   hasAlbedoAlphaMap;
        float4 albedoAlpha = SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord, hasAlbedoAlphaMap );

        [branch]
        if ( hasAlbedoAlphaMap )
        {
          color = lerp( color, albedoAlpha.rgb, albedoAlpha.a );
        }
        else
        {
          color = 0;
          ended = true;
        }
      }
      else
      {
        color = 0;
        ended = true;
      }

      origin = hitGeom.worldPosition;
    }
    else
      ended = true;
  }

  return color;
}

float3 CalcLight( float3 lightCenter, float3 worldPosition )
{
  return CastToPoint( worldPosition, lightCenter, OcclusionThreshold );
}

float3 CalcLight( float3 lightCenter, float3 worldPosition, float lightRadius, float3 cameraPosition )
{
  const float farScatterDistance = 10;
  const float minScatter         = 8;
  const float maxScatter         = HaltonSequenceLength;

  float3 px, py;
  CalcAxesForVector( normalize( lightCenter - worldPosition ), px, py );

  float pointDistance = length( worldPosition - cameraPosition );
  float dt            = saturate( pointDistance / farScatterDistance );
  int   scatter       = lerp( maxScatter, minScatter, pow( dt, 0.5 ) );

  float3 access = 0;
  for ( int secIx = 0; secIx < scatter; ++secIx )
  {
    float2 offset = haltonSequence.values[ secIx ].zw * lightRadius;
    float3 samplePos = lightCenter + px * offset.x + py * offset.y;
    access += CastToPoint( worldPosition, samplePos, OcclusionThreshold );
  }

  return access / scatter;
}

float CalcDiffuseTerm( LightCalcData lcd, float3 worldPosition, float3 worldNormal )
{
  float NdotL = saturate( dot( worldNormal, lcd.toLight ) );
  if ( lcd.hasAttenuation && NdotL > LightClippingThreshold )
  {
    float d = length( lcd.lightCenter - worldPosition );
    float t = saturate( ( d - lcd.attenuationStart ) / ( lcd.attenuationEnd - lcd.attenuationStart ) );
    NdotL *= pow( 1.0 - t, lcd.attenuationFalloff );
  }
  if ( lcd.isCone && NdotL > LightClippingThreshold )
  {
    float d = dot( -lcd.toLight, lcd.lightDirection );
    float t = saturate( ( d - lcd.phi ) / ( lcd.theta - lcd.phi ) );
    NdotL *= pow( t, lcd.coneFalloff );
  }

  return NdotL;
}

float3 CalcSpecularTerm( LightCalcData lcd, float3 worldPosition, float3 worldNormal, float3 cameraPosition, float roughness )
{
  float3 fromCamera = normalize( worldPosition - cameraPosition );
  float3 reflected  = reflect( fromCamera, worldNormal );
  float3 RdotL      = saturate( dot( lcd.toLight, reflected ) );
  return pow( RdotL, 64 ) * roughness;
}

LightCalcData CalcLightData( LightCB light, float3 worldPosition )
{
  LightCalcData lcd;
  lcd.hasAttenuation = false;
  lcd.isCone         = false;
  lcd.castShadow     = light.castShadow;

  switch ( light.type )
  {
  case LightTypeCB::Directional:
    lcd.lightCenter = worldPosition - light.direction.xyz * light.attenuationStart;
    lcd.lightRadius = light.sourceRadius;
    lcd.toLight     = -light.direction.xyz;
    break;

  case LightTypeCB::Spot:
    lcd.isCone         = true;
    lcd.theta          = light.theta;
    lcd.phi            = light.phi;
    lcd.coneFalloff    = light.coneFalloff;
    lcd.lightDirection = light.direction.xyz;
    // fall through

  case LightTypeCB::Point:
    lcd.lightCenter        = light.origin.xyz;
    lcd.lightRadius        = light.sourceRadius;
    lcd.toLight            = normalize( lcd.lightCenter - worldPosition );
    lcd.attenuationStart   = light.attenuationStart;
    lcd.attenuationEnd     = light.attenuationEnd;
    lcd.attenuationFalloff = light.attenuationFalloff;
    lcd.hasAttenuation     = true;
    break;
  }

  return lcd;
}

// https://github.com/Nadrin/PBR/blob/master/data/shaders/hlsl/pbr.hlsl

float3 CalcDirectLight( float3 albedo
                      , float roughness
                      , float metallic
                      , bool isSpecular
                      , LightCB light
                      , float3 worldPosition
                      , float3 worldNormal
                      , float3 cameraPosition
                      , float3 toCamera
                      , float3 F0
                      , float  NdotC
                      , bool   noScatter )
{
  LightCalcData lcd = CalcLightData( light, worldPosition );

  float NdotL = CalcDiffuseTerm( lcd, worldPosition, worldNormal );
  [ branch ]
  if ( NdotL <= LightClippingThreshold )
    return 0;

  float3 halfVector = normalize( lcd.toLight + toCamera );

  float3 directLighting;
  [ branch ]
  if ( isSpecular )
  {
    float3 RdotL = CalcSpecularTerm( lcd, worldPosition, worldNormal, cameraPosition, roughness );
    directLighting = ( NdotL + RdotL ) * albedo * light.color.rgb;
  }
  else
  {
    float NdotH = saturate( dot( worldNormal, halfVector ) );

    float3 fresnel = fresnelSchlick( F0, saturate( dot( halfVector, toCamera ) ) );

    // Calculate normal distribution for specular BRDF.
    float D = ndfGGX( NdotH, roughness );
    // Calculate geometric attenuation for specular BRDF.
    float G = gaSchlickGGX( NdotL, NdotC, roughness );

    // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
    // Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
    // To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
    float3 kd = lerp( 1.0 - fresnel, 0.0, metallic );

    float3 diffuseBRDF  = kd * albedo;
    float3 specularBRDF = ( fresnel * D * G ) / max( PBREpsilon, 4.0 * NdotL * NdotC );

    directLighting = ( diffuseBRDF + specularBRDF ) * light.color.rgb * NdotL;
  }

  [branch]
  if ( lcd.castShadow && light.scatterShadow && !noScatter )
    directLighting *= CalcLight( lcd.lightCenter, worldPosition, lcd.lightRadius, cameraPosition );
  else if ( lcd.castShadow )
    directLighting *= CalcLight( lcd.lightCenter, worldPosition );

  return directLighting;
}

float3 TraceDirectLighting( float3 albedo
                          , float roughness
                          , float metallic
                          , bool isSpecular
                          , float3 worldPosition
                          , float3 worldNormal
                          , float3 cameraPosition
                          , LightingEnvironmentParamsCB env
                          , bool noScatter )
{
  float3 toCamera = normalize( cameraPosition - worldPosition );
  float  NdotC    = saturate( dot( worldNormal, toCamera ) );
  float3 specRef  = 2.0 * NdotC * worldNormal - toCamera;
  float3 F0       = lerp( Fdielectric, albedo, metallic );

  float3 directLighting = 0;
  for ( int lightIx = 0; lightIx < env.lightCount; ++lightIx )
    directLighting += CalcDirectLight( albedo, roughness, metallic, isSpecular, env.sceneLights[ lightIx ], worldPosition, worldNormal, cameraPosition, toCamera, F0, NdotC, noScatter );

  return directLighting;
}