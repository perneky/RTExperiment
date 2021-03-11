#include "RootSignatures/Model.hlsli"
#include "Utils.hlsli"
#include "MaterialUtils.hlsli"

[ RootSignature( _RootSignature ) ]
VertexOutput main( VertexInput input )
{
  VertexOutput output = (VertexOutput)0;
  
  MeshParamsCB meshParams = allMeshParams[ paramIndex ];

  float4 worldPosition = mul( meshParams.worldTransform, input.position );
  float  textureScale  = GetTextureScale( meshParams );

  output.worldPosition    = worldPosition.xyz;
  output.screenPosition   = mul( meshParams.wvpTransform, input.position );
  output.clipDepth        = output.screenPosition.zw;
#ifdef HAS_OLD_POSITION
  output.prevWorldPosition = mul( meshParams.prevWorldTransform, input.oldPosition ).xyz;
  output.prevClipPosition  = mul( meshParams.prevWVPTransform, input.oldPosition );
#else
  output.prevWorldPosition = mul( meshParams.prevWorldTransform, input.position ).xyz;
  output.prevClipPosition  = mul( meshParams.prevWVPTransform, input.position );
#endif // HAS_OLD_POSITION
  output.worldNormal      = mul( (float3x3)meshParams.worldTransform, input.normal );
  output.worldTangent     = mul( (float3x3)meshParams.worldTransform, input.tangent );
  output.worldBitangent   = mul( (float3x3)meshParams.worldTransform, input.bitangent );
  output.randomValues     = meshParams.randomValues.xyz + worldPosition.xyz;
  output.texcoord         = input.texcoord * textureScale;

  return output;
}