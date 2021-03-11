#include "RootSignatures/Sprite.hlsli"

[ RootSignature( _RootSignature ) ]
VertexOutput main( VertexInput input )
{
  VertexOutput output = (VertexOutput)0;

  float4 viewPos = mul( viewTransform, input.position );
  viewPos.xy += input.size;

  output.screenPosition = mul( projTransform, viewPos );
  output.texcoord       = input.texcoord;
  output.textureId      = input.textureId;

  return output;
}