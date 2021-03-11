#define PS
#include "RootSignatures/Model.hlsli"
#include "Lighting.hlsli"

[ RootSignature( _RootSignature ) ]
[ earlydepthstencil ]
float4 main( VertexOutput input ) : SV_Target
{
  MeshParamsCB meshParams = allMeshParams[ paramIndex ];

  // Alpha test first
  float4 surfaceAlbedoAlpha = SampleAlbedoAlpha( meshParams.materialIndex, input.texcoord );
  float3 surfaceAlbedo = surfaceAlbedoAlpha.rgb;
  float  surfaceAlpha = surfaceAlbedoAlpha.a;
  
  [branch]
  if ( ShouldBeDiscared( surfaceAlpha ) )
  {
    discard;
    return 0;
  }

  // Calc surface params
  float3 randomOffset        = float3( 3, 7, 13 ) * ( float( frameParams.frameIndex ) / RandomTextureSize );
  float3 surfaceWorldNormal  = CalcSurfaceNormal( meshParams.materialIndex, input.texcoord, input.worldNormal, input.worldTangent, input.worldBitangent );
  float3 geometryWorldNormal = normalize( input.worldNormal );
  float3 emissive            = SampleEmissive( meshParams.materialIndex, input.texcoord );
  float  roughness           = SampleRoughness( meshParams.materialIndex, input.texcoord );
  float  metallic            = SampleMetallic( meshParams.materialIndex, input.texcoord );
  bool   isSpecular          = IsSpecularMaterial( meshParams.materialIndex );

  float3 probeGI = SampleGI( input.worldPosition, geometryWorldNormal, frameParams, all3DTextures[ frameParams.giSourceIndex ] );

  float3 directLighting = TraceDirectLighting( surfaceAlbedo, roughness, metallic, isSpecular, input.worldPosition, surfaceWorldNormal, frameParams.cameraPosition.xyz, lightingEnvironmentParams );

  float3 diffuseIBL  = 0;
  float3 specularIBL = 0;
  TraceIndirectLighting( probeGI, allEngineTextures[ SpecBRDFLUTSlot ], surfaceAlbedo, roughness, metallic, isSpecular, input.worldPosition, surfaceWorldNormal, frameParams.cameraPosition.xyz, lightingEnvironmentParams, diffuseIBL, specularIBL );

  float3 tracedReflection = 0;
  [branch]
  if ( any( specularIBL > 0 ) )
  {
    tracedReflection = CalcReflection( input.worldPosition
                                     , surfaceWorldNormal
                                     , frameParams
                                     , all3DTextures[ frameParams.giSourceIndex ]
                                     , lightingEnvironmentParams );
  }

  float3 lighting = emissive + directLighting + diffuseIBL.rgb + tracedReflection * specularIBL;

  return float4( lighting, surfaceAlpha );
}
