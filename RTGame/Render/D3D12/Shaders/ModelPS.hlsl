#define PS
#include "RootSignatures/Model.hlsli"
#include "Lighting.hlsli"

[ RootSignature( _RootSignature ) ]
[ earlydepthstencil ]
void main( VertexOutput input
         , bool isFrontFace : SV_IsFrontFace
         , out float4 outDirectLighting   : SV_Target0
         , out float4 outSpecularIBL      : SV_Target1
         , out float4 outReflection       : SV_Target2 
         , out float  outHQS              : SV_Target3 )
{
  const float aoFrameChainLength = 256;

  MeshParamsCB meshParams = allMeshParams[ paramIndex ];

  // Alpha test first
  float4 surfaceAlbedoAlpha = SampleAlbedoAlpha( meshParams.materialIndex, input.texcoord );
  float3 surfaceAlbedo      = surfaceAlbedoAlpha.rgb;
  float  surfaceAlpha       = surfaceAlbedoAlpha.a;

  [branch]
  if ( !IsOpaqueMaterial( meshParams.materialIndex ) && ShouldBeDiscared( surfaceAlpha ) )
  {
    discard;
    return;
  }

  float3 worldNormal = isFrontFace ? input.worldNormal : -input.worldNormal;

  float2 edgeSize = frameParams.screenSizeLQ.zw;

  // Calc surface params
  float3 surfaceWorldNormal  = CalcSurfaceNormal( meshParams.materialIndex, input.texcoord, worldNormal, input.worldTangent, input.worldBitangent );
  float3 geometryWorldNormal = normalize( worldNormal );
  float3 emissive            = SampleEmissive( meshParams.materialIndex, input.texcoord );
  float  roughness           = SampleRoughness( meshParams.materialIndex, input.texcoord );
  float  metallic            = SampleMetallic( meshParams.materialIndex, input.texcoord );
  bool   isSpecular          = IsSpecularMaterial( meshParams.materialIndex );

  HandleWetness( frameParams, input.worldPosition, meshParams.materialIndex, surfaceAlbedo, roughness, surfaceWorldNormal, geometryWorldNormal, allEngineTextures[ WetnessSlot ] );

  // Calc reprojection params
  float3 prevScreenPosition   = input.prevClipPosition.xyz / input.prevClipPosition.w;
  float2 prevScreenTexcoord   = prevScreenPosition.xy * 0.5 + 0.5;
         prevScreenTexcoord.y = 1.0 - prevScreenTexcoord.y;
  float  prevGViewDepth       = LinearizeDepth( prevScreenPosition.z, prevFrameParams.invProjTransform );
  float  prevTViewDepth       = LinearizeDepth( allEngineTextures[ prevFrameParams.depthIndex ].Sample( pointClampSampler, prevScreenTexcoord ).r, prevFrameParams.invProjTransform );
  float  prevAO               = allEngineTextures[ prevFrameParams.aoIndex ].Sample( pointClampSampler, prevScreenTexcoord ).a;
  float  prevHQS              = allEngineTextures[ prevFrameParams.hqsIndex ].Sample( clampSampler, prevScreenTexcoord ).r;
  bool   fromOutOfScreen      = prevScreenTexcoord.x <= edgeSize.x || prevScreenTexcoord.y <= edgeSize.y || prevScreenTexcoord.x >= 1.0 - edgeSize.x || prevScreenTexcoord.y > 1.0 - edgeSize.y;
  float  prevDepthD           = saturate( 1.0 - abs( prevGViewDepth - prevTViewDepth ) * 5 );
  float  prevGoodness         = prevDepthD * 0.95;

  if ( fromOutOfScreen )
    prevGoodness = 0;

  float3 probeGI = SampleGI( input.worldPosition, geometryWorldNormal, frameParams, all3DTextures[ frameParams.giSourceIndex ] );
  
  bool usePrevHQSampling = prevGoodness > 0.6;
  bool doHQSSampling     = usePrevHQSampling ? prevHQS > 0 : true;
  bool needsHQSampling   = doHQSSampling;
  float3 tracedDirectLighting = TraceDirectLighting( surfaceAlbedo, roughness, metallic, isSpecular, input.worldPosition, surfaceWorldNormal, frameParams.cameraPosition.xyz, lightingEnvironmentParams, false, needsHQSampling );

  float3 randomOffset = float( frameParams.frameIndex * RandomTextureSize / aoFrameChainLength ) / RandomTextureSize;

  float newAO = TraceOcclusion( input.worldPosition, geometryWorldNormal, input.randomValues, randomOffset, all3DTextures[ RandomTextureSlotHLSL ], 0.4, 1 );
  float ao    = lerp( newAO, prevAO, prevGoodness );

  float3 diffuseIBL  = 0;
  float3 specularIBL = 0;
  TraceIndirectLighting( probeGI, allEngineTextures[ SpecBRDFLUTSlot ], surfaceAlbedo, roughness, metallic, isSpecular, input.worldPosition, surfaceWorldNormal, frameParams.cameraPosition.xyz, lightingEnvironmentParams, diffuseIBL, specularIBL );
  diffuseIBL  *= ao;
  specularIBL *= ao;

  [branch]
  if ( any( specularIBL > 0 ) )
  {
    float3 tracedReflection = CalcReflection( input.worldPosition
                                            , surfaceWorldNormal 
                                            , frameParams
                                            , all3DTextures[ frameParams.giSourceIndex ]
                                            , lightingEnvironmentParams );
    outReflection = float4( tracedReflection, isSpecular ? 1 : roughness );
  }
  else
    outReflection = 0;

  outHQS = needsHQSampling ? 1 : 0;

  float3 toCamera = normalize( frameParams.cameraPosition.xyz - input.worldPosition.xyz );
  float  NdotC    = saturate( dot( geometryWorldNormal, toCamera ) );
  float  eadd     = meshParams.editorAddition * ( NdotC < 0.15 ? 1 : 0 );

  outSpecularIBL = 0;

  switch ( frameParams.frameDebugMode )
  {
  case FrameDebugModeCB::ShowGI:
    outDirectLighting = float4( probeGI, ao );
    break;
  case FrameDebugModeCB::ShowAO:
    outDirectLighting = float4( ao.xxx, ao );
    break;
  case FrameDebugModeCB::ShowGIAO:
    outDirectLighting = float4( probeGI * ao, ao );
    break;
  case FrameDebugModeCB::ShowSurfaceNormal:
    outDirectLighting = float4( surfaceWorldNormal * 0.5 + 0.5, ao );
    break;
  case FrameDebugModeCB::ShowGeometryNormal:
    outDirectLighting = float4( geometryWorldNormal * 0.5 + 0.5, ao );
    break;
  case FrameDebugModeCB::ShowRoughness:
    outDirectLighting = float4( roughness.xxx, ao );
    break;
  case FrameDebugModeCB::ShowMetallic:
    outDirectLighting = float4( metallic.xxx, ao );
    break;
  case FrameDebugModeCB::ShowBaseReflection:
    outDirectLighting = float4( outReflection.rgb, ao );
    break;
  case FrameDebugModeCB::ShowNeedHQLightSampling:
    outDirectLighting.rgb = doHQSSampling ? 0.75 : 0.25;
    outDirectLighting.a   = ao;
    break;
  default:
    outDirectLighting = float4( emissive + tracedDirectLighting + diffuseIBL + eadd, ao );
    outSpecularIBL    = float4( specularIBL, 1 );
    break;
  }
}
