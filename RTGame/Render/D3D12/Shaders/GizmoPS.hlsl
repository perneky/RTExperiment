#define PS
#include "RootSignatures/Gizmo.hlsli"

[ RootSignature( _RootSignature ) ]
float4 main( VertexOutput input ) : SV_Target
{
  return input.color;
}
