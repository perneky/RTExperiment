#define PS
#include "RootSignatures/Depth.hlsli"
#include "MaterialUtils.hlsli"

[ RootSignature( _RootSignature ) ]
void main( VertexOutput input
#ifdef USE_MOTION_VECTORS
         , out float2 motion : SV_Target0
#endif // USE_MOTION_VECTORS
         )
{
  MeshParamsCB meshParams = allMeshParams[ paramIndex ];

  float4 surfaceAlbedoAlpha = SampleAlbedoAlpha( meshParams.materialIndex, input.texcoord );
  float3 surfaceAlbedo      = surfaceAlbedoAlpha.rgb;
  float  surfaceAlpha       = surfaceAlbedoAlpha.a;

  if ( !IsOpaqueMaterial( meshParams.materialIndex ) && ShouldBeDiscared( surfaceAlpha ) )
    discard;

#ifdef USE_MOTION_VECTORS
  float3 screenPosition = input.clipPosition.xyz / input.clipPosition.w;
  float2 screenTexcoord = screenPosition.xy * 0.5 + 0.5;
  screenTexcoord.y = 1.0 - screenTexcoord.y;
  float2 posInTexels    = screenTexcoord * frameParams.screenSizeLQ.xy;

  float3 prevScreenPosition = input.prevClipPosition.xyz / input.prevClipPosition.w;
  float2 prevScreenTexcoord = prevScreenPosition.xy * 0.5 + 0.5;
  prevScreenTexcoord.y = 1.0 - prevScreenTexcoord.y;
  float2 prevPosInTexels    = prevScreenTexcoord * frameParams.screenSizeLQ.xy;
  
  motion = prevPosInTexels - posInTexels;
#endif // USE_MOTION_VECTORS
}
