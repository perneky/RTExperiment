#pragma once

#include "Utils.hlsli"

bool IsOpaqueMaterial( int materialIndex )
{
  return allMaterials.mat[ materialIndex ].args.w == 0;
}

bool IsOneBitAlphaMaterial( int materialIndex )
{
  return allMaterials.mat[ materialIndex ].args.w == 1;
}

bool IsTranslucentMaterial( int materialIndex )
{
  return allMaterials.mat[ materialIndex ].args.w == 2;
}

bool IsFlipNormalXMaterial( int materialIndex )
{
  return ( allMaterials.mat[ materialIndex ].args.x & MaterialFeatures::FlipNormalX ) != 0;
}

bool IsSpecularMaterial( int materialIndex )
{
  return ( allMaterials.mat[ materialIndex ].args.x & MaterialFeatures::UseSpecular ) != 0;
}

bool IsScaleUVMaterial( int materialIndex )
{
  return ( allMaterials.mat[ materialIndex ].args.x & MaterialFeatures::ScaleUV ) != 0;
}

bool IsClampToEdgeMaterial( int materialIndex )
{
  return ( allMaterials.mat[ materialIndex ].args.x & MaterialFeatures::ClampToEdge ) != 0;
}

bool IsUseWetnessMaterial( int materialIndex )
{
  return ( allMaterials.mat[ materialIndex ].args.x & MaterialFeatures::UseWetness ) != 0;
}

bool IsTwoSidedMaterial( int materialIndex )
{
  return ( allMaterials.mat[ materialIndex ].args.x & MaterialFeatures::TwoSided ) != 0;
}

float4 SampleMaterialTextureInternal( Texture2D tex, uint materialIndex, float2 texcoord, float mipLevel )
{
  if ( IsClampToEdgeMaterial( materialIndex ) )
    return tex.SampleLevel( clampSampler, texcoord, mipLevel );
  else
    return tex.SampleLevel( wrapSampler, texcoord, mipLevel );
}

float4 SampleMaterialTextureInternal( Texture2D tex, uint materialIndex, float2 texcoord )
{
#ifdef IN_COMPUTE
  return SampleMaterialTextureInternal( tex, materialIndex, texcoord, 0 );
#else
  if ( IsClampToEdgeMaterial( materialIndex ) )
    return tex.Sample( clampSampler, texcoord );
  else
    return tex.Sample( wrapSampler, texcoord );
#endif // IN_COMPUTE
}

float4 SampleAlbedoAlpha( int materialIndex, float2 texcoord, float mipLevel )
{
  int    albedoIndex = allMaterials.mat[ materialIndex ].textureIndices.x;
  float4 albedoAlpha = 1;

  [branch]
  if ( albedoIndex > -1 )
  {
    albedoAlpha     = SampleMaterialTextureInternal( allMaterialTextures[ albedoIndex ], materialIndex, texcoord, mipLevel );
    albedoAlpha.rgb = FromsRGB( albedoAlpha.rgb );
  }

  return albedoAlpha;
}

float4 SampleAlbedoAlpha( int materialIndex, float2 texcoord, out bool hasValue )
{
  int    albedoIndex = allMaterials.mat[ materialIndex ].textureIndices.x;
  float4 albedoAlpha = 1;

  [branch]
  if ( albedoIndex > -1 )
  {
    albedoAlpha     = SampleMaterialTextureInternal( allMaterialTextures[ albedoIndex ], materialIndex, texcoord );
    albedoAlpha.rgb = FromsRGB( albedoAlpha.rgb );
    hasValue        = true;
  }
  else
    hasValue = false;

  return albedoAlpha;
}

float4 SampleAlbedoAlpha( int materialIndex, float2 texcoord )
{
  bool hasValue;
  return SampleAlbedoAlpha( materialIndex, texcoord, hasValue );
}

float3 CalcSurfaceNormal( int materialIndex, float2 texcoord, float3 worldNormal, float3 worldTangent, float3 worldBitangent )
{
  int normalIndex = allMaterials.mat[ materialIndex ].textureIndices.y;

  float3 geometryWorldNormal = normalize( worldNormal );
  [branch]
  if ( normalIndex > -1 )
  {
    bool     flipNormalX   = IsFlipNormalXMaterial( materialIndex );
    float3x3 toWorldSpace  = float3x3( -worldBitangent, worldTangent, worldNormal );
    float3   surfaceNormal = float3( SampleMaterialTextureInternal( allMaterialTextures[ normalIndex ], materialIndex, texcoord ).rg * 2 - 1, 0 );
    surfaceNormal.z = sqrt( 1.0f - dot( surfaceNormal.xy, surfaceNormal.xy ) );
    if ( flipNormalX )
      surfaceNormal.x = -surfaceNormal.x;

    worldNormal = normalize( mul( surfaceNormal, toWorldSpace ) );
  }
  else
    worldNormal = geometryWorldNormal;

  return worldNormal;
}

float SampleRoughness( int materialIndex, float2 texcoord )
{
  int   roughnessIndex = allMaterials.mat[ materialIndex ].textureIndices.w;
  float roughness = 0;

  [branch]
  if ( roughnessIndex > -1 )
    roughness = SampleMaterialTextureInternal( allMaterialTextures[ roughnessIndex ], materialIndex, texcoord ).r;
  else
    roughness = allMaterials.mat[ materialIndex ].textureValues.y;

  return roughness;
}

float SampleRoughness( int materialIndex, float2 texcoord, float mipLevel )
{
  int   roughnessIndex = allMaterials.mat[ materialIndex ].textureIndices.w;
  float roughness = 0;

  [branch]
  if ( roughnessIndex > -1 )
    roughness = SampleMaterialTextureInternal( allMaterialTextures[ roughnessIndex ], materialIndex, texcoord, mipLevel ).r;
  else
    roughness = allMaterials.mat[ materialIndex ].textureValues.y;

  return roughness;
}

float SampleMetallic( int materialIndex, float2 texcoord )
{
  int   metallicIndex = allMaterials.mat[ materialIndex ].textureIndices.z;
  float metallic = 0;

  [branch]
  if ( metallicIndex > -1 )
    metallic = SampleMaterialTextureInternal( allMaterialTextures[ metallicIndex ], materialIndex, texcoord ).r;
  else
    metallic = allMaterials.mat[ materialIndex ].textureValues.x;

  return metallic;
}

float SampleMetallic( int materialIndex, float2 texcoord, float mipLevel )
{
  int   metallicIndex = allMaterials.mat[ materialIndex ].textureIndices.z;
  float metallic = 0;

  [branch]
  if ( metallicIndex > -1 )
    metallic = SampleMaterialTextureInternal( allMaterialTextures[ metallicIndex ], materialIndex, texcoord, mipLevel ).r;
  else
    metallic = allMaterials.mat[ materialIndex ].textureValues.x;

  return metallic;
}

float3 SampleEmissive( int materialIndex, float2 texcoord )
{
  int    emissiveIndex = allMaterials.mat[ materialIndex ].args.y;
  float3 emissive      = 0;

  [branch]
  if ( emissiveIndex > -1 )
    emissive = SampleMaterialTextureInternal( allMaterialTextures[ emissiveIndex ], materialIndex, texcoord ).r;

  return emissive;
}

float3 SampleEmissive( int materialIndex, float2 texcoord, float mipLevel )
{
  int    emissiveIndex = allMaterials.mat[ materialIndex ].args.y;
  float3 emissive = 0;

  [branch]
  if ( emissiveIndex > -1 )
    emissive = SampleMaterialTextureInternal( allMaterialTextures[ emissiveIndex ], materialIndex, texcoord, mipLevel ).r;

  return emissive;
}

float GetTextureScale( MeshParamsCB meshParams )
{
  return IsScaleUVMaterial( meshParams.materialIndex ) ? length( float3( meshParams.worldTransform._11, meshParams.worldTransform._22, meshParams.worldTransform._33 ) ) : 1;
}

void HandleWetness( FrameParamsCB frameParams, float3 worldPosition, int materialIndex, inout float3 albedo, inout float roughness, inout float3 normal, float3 geomNormal, Texture2D wetnessTexture )
{
  [branch]
  if ( !IsUseWetnessMaterial( materialIndex ) )
    return;

  float2 wetnessUV = ( worldPosition.xz - frameParams.wetnessOriginInvSize.xy ) * frameParams.wetnessOriginInvSize.zw;
  float  wetness   = wetnessTexture.Sample( clampSampler, wetnessUV ).r;

  albedo    = lerp( albedo, albedo * 0.1284, wetness );
  roughness = lerp( roughness, 0, wetness );
  normal    = normalize( lerp( normal, geomNormal, wetness ) );
}