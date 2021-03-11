#include "RootSignatures/Gizmo.hlsli"

[ RootSignature( _RootSignature ) ]
VertexOutput main( VertexInput input )
{
  VertexOutput output = (VertexOutput)0;
  
  output.screenPosition = mul( meshParams.wvpTransform, input.position );
  output.color.rgb      = input.color.rgb;
  output.color.a        = ( uint( round( input.color.a * 255 ) ) == selectedGizmo ) ? 1 : 0.25;
  
  return output;
}