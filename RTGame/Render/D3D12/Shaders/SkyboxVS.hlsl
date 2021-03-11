#include "RootSignatures/Skybox.hlsli"

[ RootSignature( _RootSignature ) ]
VertexOutput main( VertexInput input )
{
  VertexOutput output = (VertexOutput)0;
  
  float4 projectedPosition = mul( frameParams.projTransform, float4( mul( (float3x3)frameParams.viewTransform, input.position.xyz ), 1.0 ) );
  projectedPosition.z = projectedPosition.w;
  
  output.screenPosition = projectedPosition;
  output.texcoord       = input.position.xyz;

  return output;
}