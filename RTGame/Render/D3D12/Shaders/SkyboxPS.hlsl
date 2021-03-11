#define PS
#include "RootSignatures/Skybox.hlsli"

[ RootSignature( _RootSignature ) ]
float4 main( VertexOutput input ) : SV_Target
{
  MeshParamsCB meshParams = allMeshParams[ paramIndex ];

  int albedoIndex = allMaterials.mat[ meshParams.materialIndex ].textureIndices.x;
  
  float3 surfaceAlbedo = 1;
  [branch]
  if ( albedoIndex > -1 )
    surfaceAlbedo = allCubeTextures[ albedoIndex ].Sample( skySampler, input.texcoord ).rgb;

  return float4( surfaceAlbedo.rgb, 1 );
}
