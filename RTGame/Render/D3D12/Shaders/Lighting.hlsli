#include "../../ShaderValues.h"
#include "MaterialUtils.hlsli"
#include "RTUtils.hlsli"
#include "GI.hlsli"
#include "PBRUtils.hlsli"
#include "TraceDirectLighting.hlsli"

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

  float  visibility = 1;
  float3 tint       = 1;
  bool   ended      = false;

  float3 origin = worldPosition;

  HitGeometry hitGeom;

  while ( !ended && visibility > 0 )
  {
    hitGeom = TraceRay( origin, toReflection, castMinDistance, 1000 );

    [branch]
    if ( hitGeom.t >= 0 )
    {
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

      origin = hitGeom.worldPosition;
    }
    else
      return allCubeTextures[ allMaterials.mat[ env.skyMaterial ].textureIndices.x ].SampleLevel( wrapSampler, toReflection, 0 ).rgb;
  }

  float3 surfaceWorldNormal = CalcSurfaceNormal( hitGeom.materialIndex, hitGeom.texcoord, hitGeom.worldNormal, hitGeom.worldTangent, hitGeom.worldBitangent );
  float3 surfaceAlbedo      = SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord ).rgb;
  float3 surfaceEmissive    = SampleEmissive( hitGeom.materialIndex, hitGeom.texcoord );
  float  surfaceRoughness   = SampleRoughness( hitGeom.materialIndex, hitGeom.texcoord );
  float  surfaceMetallic    = SampleMetallic( hitGeom.materialIndex, hitGeom.texcoord );
  bool   surfaceIsSpecular  = IsSpecularMaterial( hitGeom.materialIndex );

  float3 probeGI          = SampleGI( hitGeom.worldPosition, hitGeom.worldNormal, frameParams, giTexture );
  float3 effectiveAlbedo  = lerp( tint, surfaceAlbedo, visibility );
  float3 directLighting   = TraceDirectLighting( effectiveAlbedo, surfaceRoughness, surfaceMetallic, surfaceIsSpecular, hitGeom.worldPosition, surfaceWorldNormal, worldPosition, env, true );

  float3 diffuseIBL;
  float3 specularIBL;
  TraceIndirectLighting( probeGI, allEngineTextures[ SpecBRDFLUTSlot ], effectiveAlbedo, surfaceRoughness, surfaceMetallic, hitGeom.materialIndex, hitGeom.worldPosition, surfaceWorldNormal, frameParams.cameraPosition.xyz, lightingEnvironmentParams, diffuseIBL, specularIBL );
  return surfaceEmissive + directLighting + diffuseIBL;
}

float TraceOcclusionForOffset( float3 origin, float3 geometryWorldNormal, float3 offset, float3 px, float3 py, float range )
{
  HitGeometry hitGeom = TraceRay( origin + geometryWorldNormal * OcclusionThreshold
                                , PertubNormal( geometryWorldNormal, px, py, offset )
                                , castMinDistance
                                , range );

  [branch]
  if ( hitGeom.t >= 0 )
  {
    [branch]
    if ( IsOpaqueMaterial( hitGeom.materialIndex ) )
      return saturate( hitGeom.t );
    else
      return saturate( 1.0 - SampleAlbedoAlpha( hitGeom.materialIndex, hitGeom.texcoord ).a - hitGeom.t );
  }

  return 1;
}

float TraceOcclusion( float3 worldPosition, float3 geometryWorldNormal, float3 randomValues, float3 randomOffset, Texture3D randomTexture, float range, int quality )
{
  float contribution = 0;

  float3 px, py;
  CalcAxesForVector( geometryWorldNormal, px, py );

  for ( int rayIx = 0; rayIx < quality; ++rayIx )
  {
    float3 offset = GetNextRandomOffset( randomTexture, noiseSampler, randomOffset, randomValues );
    contribution += TraceOcclusionForOffset( worldPosition, geometryWorldNormal, offset, px, py, range );

    offset.xy = -offset.xy;
    contribution += TraceOcclusionForOffset( worldPosition, geometryWorldNormal, offset, px, py, range );

    float x = offset.x;
    offset.x = -offset.y;
    offset.y = x;
    contribution += TraceOcclusionForOffset( worldPosition, geometryWorldNormal, offset, px, py, range );

    offset.xy = -offset.xy;
    contribution += TraceOcclusionForOffset( worldPosition, geometryWorldNormal, offset, px, py, range );
  }

  contribution /= quality * 4;

  return pow( contribution, 1.5 );
}
