#define PS
#include "RootSignatures/GIProbe.hlsli"
#include "Utils.hlsli"

[ RootSignature( _RootSignature ) ]
float4 main( VertexOutput input ) : SV_Target0
{
  int3 wix = GIProbeWorldIndex( input.probeId, frameParams );
  return all3DTextures[ frameParams.giSourceIndex ].Load( int4( wix, 0 ) );
}
