#define PS
#include "RootSignatures/Sprite.hlsli"

[ RootSignature( _RootSignature ) ]
float4 main( VertexOutput input ) : SV_Target0
{
  return allMaterialTextures[ input.textureId ].Sample( linearSampler, input.texcoord );
}
