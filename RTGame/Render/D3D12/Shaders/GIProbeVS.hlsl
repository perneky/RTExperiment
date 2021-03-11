#include "RootSignatures/GIProbe.hlsli"
#include "Utils.hlsli"

[ RootSignature( _RootSignature ) ]
VertexOutput main( VertexInput input, uint probeId : SV_InstanceID )
{
  VertexOutput output = (VertexOutput)0;
  
  float3 worldPosition = GIProbePosition( probeId, frameParams ) + input.position.xyz * 0.2f;

  output.worldPosition    = worldPosition.xyz;
  output.worldNormal      = input.position.xyz;
  output.screenPosition   = mul( vpTransform, float4( worldPosition, 1 ) );
  output.probeId          = probeId;

  return output;
}