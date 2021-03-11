#include "RootSignatures/Depth.hlsli"
#include "Utils.hlsli"
#include "MaterialUtils.hlsli"

[ RootSignature( _RootSignature ) ]
VertexOutput main( VertexInput input )
{
  VertexOutput output = (VertexOutput)0;

  MeshParamsCB meshParams = allMeshParams[ paramIndex ];

  float textureScale = GetTextureScale( meshParams );

  output.screenPosition = mul( meshParams.wvpTransform, input.position );
  output.texcoord       = input.texcoord * textureScale;
  output.worldNormal    = mul( ( float3x3 )meshParams.worldTransform, input.normal );

  return output;
}