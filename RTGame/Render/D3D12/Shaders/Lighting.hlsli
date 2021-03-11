#include "../../ShaderValues.h"
#include "MaterialUtils.hlsli"
#include "RTUtils.hlsli"
#include "GI.hlsli"
#include "PBRUtils.hlsli"

static const float lightClippingThreshold = 0.001;

float3 CastToPoint( float3 worldPosition, float3 pointCoord )
{
  float3 color = 1;

#if ENABLE_RAYTRACING_FOR_RENDER
  float3 toLight = pointCoord - worldPosition;

  ClosestOpaqueQuery query;

  RayDesc ray;
  ray.Origin    = worldPosition;
  ray.TMin      = castMinDistance;
  ray.TMax      = length( toLight );
  ray.Direction = normalize( toLight );

  bool ended = false;
  while ( !ended && any( color > 0 ) )
  {
    query.TraceRayInline( rayTracingScene, ClosestOpaqueQueryFlags, 0xFF, ray );
    query.Proceed();
    if ( query.CommittedStatus() == COMMITTED_TRIANGLE_HIT )
    {
      HitGeometry hitGeom = CalcHitGeometry( query );
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

      ray.Origin = hitGeom.worldPosition;
    }
    else
      ended = true;
  }
#endif // ENABLE_RAYTRACING_FOR_RENDER

  return color;
}

float3 CalcLight( float3 lightCenter, float3 toLight, float lightRadius, float3 worldPosition )
{
  return CastToPoint( worldPosition, lightCenter );
}

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
    // falloff

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

float CalcDiffuseTerm( LightCalcData lcd, float3 worldPosition, float3 worldNormal )
{
  float NdotL = saturate( dot( worldNormal, lcd.toLight ) );
  if ( lcd.hasAttenuation && NdotL > lightClippingThreshold )
  {
    float d = length( lcd.lightCenter - worldPosition );
    float t = saturate( ( d - lcd.attenuationStart ) / ( lcd.attenuationEnd - lcd.attenuationStart ) );
    NdotL *= pow( 1.0 - t, lcd.attenuationFalloff );
  }
  if ( lcd.isCone && NdotL > lightClippingThreshold )
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
                      , float  NdotC )
{
  LightCalcData lcd = CalcLightData( light, worldPosition );

  float NdotL = CalcDiffuseTerm( lcd, worldPosition, worldNormal );
  [ branch ]
  if ( NdotL <= lightClippingThreshold )
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
  if ( lcd.castShadow )
    directLighting *= CalcLight( lcd.lightCenter, lcd.toLight, lcd.lightRadius, worldPosition );

  return directLighting;
}

float3 TraceDirectLighting( float3 albedo
                          , float roughness
                          , float metallic
                          , bool isSpecular
                          , float3 worldPosition
                          , float3 worldNormal
                          , float3 cameraPosition
                          , LightingEnvironmentParamsCB env )
{
  float3 toCamera = normalize( cameraPosition - worldPosition );
  float  NdotC    = saturate( dot( worldNormal, toCamera ) );
  float3 specRef  = 2.0 * NdotC * worldNormal - toCamera;
  float3 F0       = lerp( Fdielectric, albedo, metallic );

  float3 directLighting = 0;
  for ( int lightIx = 0; lightIx < env.lightCount; ++lightIx )
    directLighting += CalcDirectLight( albedo, roughness, metallic, isSpecular, env.sceneLights[ lightIx ], worldPosition, worldNormal, cameraPosition, toCamera, F0, NdotC );

  return directLighting;
}

void TraceIndirectLighting( float3 gi
                          , Texture2D specBRDFLUT
                          , float3 albedo
                          , float roughness
                          , float metallic
                          , bool isSpecular
                          , float3 worldPosition
                          , float3 worldNormal
                          , float3 cameraPosition
                          , LightingEnvironmentParamsCB env
                          , out float3 diffuseIBL
                          , out float3 specularIBL )
{
  [branch]
  if ( isSpecular )
  {
    diffuseIBL  = albedo * gi;
    specularIBL = albedo;
  }
  else
  {
    float3 toCamera     = normalize( cameraPosition - worldPosition );
    float  NdotC        = saturate( dot( worldNormal, toCamera ) );
    float3 specRef      = 2.0 * NdotC * worldNormal - toCamera;
    float3 F0           = lerp( Fdielectric, albedo, metallic );
    float3 fresnel      = fresnelSchlick( F0, NdotC );
    float3 kd           = lerp( 1.0 - fresnel, 0.0, metallic );
    float2 specularBRDF = specBRDFLUT.Sample( clampSampler, float2( NdotC, roughness ) ).rg;
  
    diffuseIBL  = kd * albedo * gi;
    specularIBL = F0 * specularBRDF.x + specularBRDF.y;
  }

}

float3 CalcReflection( float3 worldPosition, float3 worldNormal, FrameParamsCB frameParams, Texture3D giTexture, LightingEnvironmentParamsCB env )
{
  float3 toCamera     = normalize( frameParams.cameraPosition.xyz - worldPosition );
  float3 toReflection = reflect( -toCamera, worldNormal );

#if ENABLE_RAYTRACING_FOR_RENDER
  RayDesc ray;
  ray.Origin    = worldPosition;
  ray.TMin      = castMinDistance;
  ray.TMax      = 1000;
  ray.Direction = toReflection;

  ClosestOpaqueQuery query;
  HitGeometry        hitGeom;

  float  visibility = 1;
  float3 tint       = 1;
  bool   ended      = false;
  while ( !ended && visibility > 0 )
  {
    query.TraceRayInline( rayTracingScene, ClosestOpaqueQueryFlags, 0xFF, ray );
    query.Proceed();

    if ( query.CommittedStatus() == COMMITTED_TRIANGLE_HIT )
    {
      hitGeom = CalcHitGeometry( query );

      [branch]
      if ( IsOneBitAlphaMaterial( hitGeom.materialIndex ) )
      {
        float alpha = SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord ).a;
        [branch]
        if ( !ShouldBeDiscared( alpha ) )
          ended = true;
      }
      else if ( IsTranslucentMaterial( hitGeom.materialIndex ) )
      {
        bool   hasAlbedoAlphaMap;
        float4 albedoAlpha = SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord, hasAlbedoAlphaMap );

        [branch]
        if ( hasAlbedoAlphaMap )
        {
          visibility *= 1.0 - albedoAlpha.a;
          tint *= lerp( 1, albedoAlpha.rgb, albedoAlpha.a );
        }
        else
          ended = true;
      }
      else
        ended = true;

      ray.Origin = hitGeom.worldPosition;
    }
    else
      return allCubeTextures[ allMaterials.mat[ env.skyMaterial ].textureIndices.x ].SampleLevel( wrapSampler, ray.Direction, 0 ).rgb;
  }

  float3 surfaceWorldNormal = CalcSurfaceNormal( hitGeom.materialIndex, hitGeom.texcoord, hitGeom.worldNormal, hitGeom.worldTangent, hitGeom.worldBitangent );
  float3 surfaceAlbedo      = SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord ).rgb;
  float3 surfaceEmissive    = SampleEmissive( hitGeom.materialIndex, hitGeom.texcoord );
  float  surfaceRoughness   = SampleRoughness( hitGeom.materialIndex, hitGeom.texcoord );
  float  surfaceMetallic    = SampleMetallic( hitGeom.materialIndex, hitGeom.texcoord );
  bool   surfaceIsSpecular  = IsSpecularMaterial( hitGeom.materialIndex );

  float3 probeGI          = SampleGI( hitGeom.worldPosition, hitGeom.worldNormal, frameParams, giTexture );
  float3 effectiveAlbedo  = lerp( tint, surfaceAlbedo, visibility );
  float3 directLighting   = TraceDirectLighting( effectiveAlbedo, surfaceRoughness, surfaceMetallic, surfaceIsSpecular, hitGeom.worldPosition, surfaceWorldNormal, worldPosition, env );

  surfaceEmissive *= 1.0 - saturate( query.CommittedRayT() / 5 );
  
  float3 diffuseIBL;
  float3 specularIBL;
  TraceIndirectLighting( probeGI, allEngineTextures[ SpecBRDFLUTSlot ], effectiveAlbedo, surfaceRoughness, surfaceMetallic, hitGeom.materialIndex, hitGeom.worldPosition, surfaceWorldNormal, frameParams.cameraPosition.xyz, lightingEnvironmentParams, diffuseIBL, specularIBL );
  return surfaceEmissive + directLighting + diffuseIBL;
#else
  return allCubeTextures[ allMaterials.mat[ env.skyMaterial ].textureIndices.x ].SampleLevel( wrapSampler, toReflection, 0 ).rgb;
#endif // ENABLE_RAYTRACING_FOR_RENDER
}

float TraceOcclusionForOffset( ClosestOpaqueQuery query, RayDesc ray, float3 geometryWorldNormal, float3 offset, float3 px, float3 py )
{
  ray.Direction = PertubNormal( geometryWorldNormal, px, py, offset );

  query.TraceRayInline( rayTracingScene, ClosestOpaqueQueryFlags, 0xFF, ray );
  query.Proceed();

  float visibility = 1;

  [branch]
  if ( query.CommittedStatus() == COMMITTED_TRIANGLE_HIT )
  {
    HitGeometry hitGeom = CalcHitGeometry( query );

    [branch]
    if ( IsOpaqueMaterial( hitGeom.materialIndex ) )
      visibility -= 1.0 - query.CommittedRayT();
    else
      visibility -= SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord ).a - query.CommittedRayT();
  }

  return saturate( visibility );
}

float TraceOcclusion( float3 worldPosition, float3 geometryWorldNormal, float3 randomValues, float3 randomOffset, Texture3D randomTexture, float range, int quality )
{
#if ENABLE_RAYTRACING_FOR_RENDER
  ClosestOpaqueQuery query;
  float contribution = 0;

  float3 px, py;
  CalcAxesForVector( geometryWorldNormal, px, py );

  RayDesc ray;
  ray.Origin = worldPosition;
  ray.TMin   = castMinDistance;
  ray.TMax   = range;

  for ( int rayIx = 0; rayIx < quality; ++rayIx )
  {
    float3 offset = GetNextRandomOffset( randomTexture, noiseSampler, randomOffset, randomValues );
    contribution += TraceOcclusionForOffset( query, ray, geometryWorldNormal, offset, px, py );

    offset.xy = -offset.xy;
    contribution += TraceOcclusionForOffset( query, ray, geometryWorldNormal, offset, px, py );

    float x = offset.x;
    offset.x = -offset.y;
    offset.y = x;
    contribution += TraceOcclusionForOffset( query, ray, geometryWorldNormal, offset, px, py );

    offset.xy = -offset.xy;
    contribution += TraceOcclusionForOffset( query, ray, geometryWorldNormal, offset, px, py );
  }

  contribution /= quality * 4;

  return pow( contribution, 1.5 );
#else
  return 1;
#endif // ENABLE_RAYTRACING_FOR_RENDER
}
