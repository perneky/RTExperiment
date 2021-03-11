#define PS
#include "RootSignatures/Depth.hlsli"
#include "MaterialUtils.hlsli"

[ RootSignature( _RootSignature ) ]
void main( VertexOutput input )
{
  MeshParamsCB meshParams = allMeshParams[ paramIndex ];

  float4 surfaceAlbedoAlpha = SampleAlbedoAlpha( meshParams.materialIndex, input.texcoord );
  float3 surfaceAlbedo      = surfaceAlbedoAlpha.rgb;
  float  surfaceAlpha       = surfaceAlbedoAlpha.a;

  if ( !IsOpaqueMaterial( meshParams.materialIndex ) && ShouldBeDiscared( surfaceAlpha ) )
    discard;
}
