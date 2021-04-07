#define IN_COMPUTE

#include "RootSignatures/ShaderStructures.hlsli"
#include "../../ShaderValues.h"

#define _RootSignature "RootFlags( 0 )," \
                       "RootConstants( b0, num32BitConstants = 1 )," \
                       "CBV( b1 )," \
                       "CBV( b2 )," \
                       "CBV( b3 )," \
                       "CBV( b4 )," \
                       "DescriptorTable( UAV( u0, flags = DATA_VOLATILE ) )," \
                       "DescriptorTable( SRV( t0, offset = " RTSceneBaseSlotStr            ", numDescriptors = " RTSceneCountStr            ", flags = DESCRIPTORS_VOLATILE, space = 0 )," \
                       "                 SRV( t1, offset = " Engine2DResourceBaseSlotStr   ", numDescriptors = " Engine2DResourceCountStr   ", flags = DESCRIPTORS_VOLATILE, space = 1 )," \
                       "                 SRV( t2, offset = " VaryingResourceBaseSlotStr    ", numDescriptors = " VaryingResourceCountStr    ", flags = DESCRIPTORS_VOLATILE, space = 2 )," \
                       "                 SRV( t3, offset = " EngineCubeResourceBaseSlotStr ", numDescriptors = " EngineCubeResourceCountStr ", flags = DESCRIPTORS_VOLATILE, space = 3 )," \
                       "                 SRV( t4, offset = " EngineVolResourceBaseSlotStr  ", numDescriptors = " EngineVolResourceCountStr  ", flags = DESCRIPTORS_VOLATILE, space = 4 )," \
                       "                 SRV( t5, offset = " CBVIBBaseSlotStr              ", numDescriptors = " CBVIBCountStr              ", flags = DESCRIPTORS_VOLATILE, space = 5 )," \
                       "                 SRV( t6, offset = " CBVVBBaseSlotStr              ", numDescriptors = " CBVVBCountStr              ", flags = DESCRIPTORS_VOLATILE, space = 6 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_ANISOTROPIC," \
                       "               addressU = TEXTURE_ADDRESS_WRAP," \
                       "               addressV = TEXTURE_ADDRESS_WRAP," \
                       "               addressW = TEXTURE_ADDRESS_WRAP," \
                       "               maxAnisotropy = 16 )," \
                       "StaticSampler( s1," \
                       "               filter = FILTER_ANISOTROPIC," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP," \
                       "               maxAnisotropy = 16 )," \
                       "StaticSampler( s2," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_WRAP," \
                       "               addressV = TEXTURE_ADDRESS_WRAP," \
                       "               addressW = TEXTURE_ADDRESS_WRAP )," \
                       "StaticSampler( s3," \
                       "               filter = FILTER_MIN_MAG_MIP_POINT," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \
                       ""

cbuffer Params : register( b0 )
{
  int startProbeId;
};

cbuffer LightingEnvironmentParams : register( b1 )
{
  LightingEnvironmentParamsCB lightingEnvironmentParams;
};

cbuffer FrameParams : register( b2 )
{
  FrameParamsCB frameParams;
};

cbuffer PrevFrameParams : register( b3 )
{
  FrameParamsCB prevFrameParams;
};

ConstantBuffer< AllMaterialsCB >  allMaterials  : register( b4 );

RWTexture3D< float4 > destination : register( u0 );

RaytracingAccelerationStructure rayTracingScenes[] : register( t0, space0 );

Texture2D    allEngineTextures[]   : register( t1, space1 );
Texture2D    allMaterialTextures[] : register( t2, space2 );
TextureCube  allCubeTextures[]     : register( t3, space3 );
Texture3D    all3DTextures[]       : register( t4, space4 );
SamplerState wrapSampler           : register( s0 );
SamplerState clampSampler          : register( s1 );
SamplerState noiseSampler          : register( s2 );
SamplerState pointClampSampler     : register( s3 );

ByteAddressBuffer                  meshIndices [ MaxMeshCount ] : register( t5, space5 );
StructuredBuffer< RTVertexFormat > meshVertices[ MaxMeshCount ] : register( t6, space6 );

#include "Lighting.hlsli"

groupshared uint avg[ 3 ];

static const float3 nDir[ 6 ] =
{
  float3( -1,  0,  0 ),
  float3(  1,  0,  0 ),
  float3(  0, -1,  0 ),
  float3(  0,  1,  0 ),
  float3(  0,  0, -1 ),
  float3(  0,  0,  1 ),
};

static const int3 nDirI[ 6 ] =
{
  int3( -1,  0,  0 ),
  int3(  1,  0,  0 ),
  int3(  0, -1,  0 ),
  int3(  0,  1,  0 ),
  int3(  0,  0, -1 ),
  int3(  0,  0,  1 ),
};

[RootSignature( _RootSignature )]
[numthreads( GIProbeKernelWidth, GIProbeKernelHeight, 1 )]
void main( uint3 groupThreadID : SV_GroupThreadID, uint3 groupId : SV_GroupID )
{
  avg[ 0 ] = avg[ 1 ] = avg[ 2 ] = 0;

  float2 ts = float2( GIProbeKernelWidth, GIProbeKernelHeight );
  float2 uv = ( float2( groupThreadID.xy ) + 0.5 ) / ts;

  int probeId = ( startProbeId + groupId.x ) % ( frameParams.giProbeCount.w * frameParams.giProbeCount.y );

  float3 worldNormal   = UVToDirectionOctahedral( uv );
  float3 worldPosition = GIProbePosition( probeId, frameParams );

  float3 randomOffset = float3( 3, 7, 13 ) * ( float( frameParams.frameIndex ) / RandomTextureSize );

  float3 gi = TraceGI( worldPosition
                     , worldNormal
                     , lightingEnvironmentParams
                     , frameParams );
  
  GroupMemoryBarrierWithGroupSync();

  uint3 intGI = uint3( gi * 1000 );
  InterlockedAdd( avg[ 0 ], intGI.x );
  InterlockedAdd( avg[ 1 ], intGI.y );
  InterlockedAdd( avg[ 2 ], intGI.z );

  // Wait until the 'avg' is calculated
  GroupMemoryBarrierWithGroupSync();

  [branch]
  if ( all( groupThreadID.xy == 0 ) )
  {
    float3 avgValue   = ( float3( avg[ 0 ], avg[ 1 ], avg[ 2 ] ) / 1000 ) / ( GIProbeWidth * GIProbeHeight );
    float3 neighbours = 0;
    float  ncount     = 0;
  
    int3 wix = GIProbeWorldIndex( probeId, frameParams );

    for ( int rayIx = 0; rayIx < 6; rayIx++ )
    {
      int3 nix = wix + nDirI[ rayIx ];
      [branch]
      if ( any( nix < 0 ) || any( nix >= frameParams.giProbeCount.xyz ) )
        continue;

      HitGeometry hitGeom = TraceRay( worldPosition, nDir[ rayIx ] * GIProbeSpacing, castMinDistance, 1 );

      [branch]
      if ( hitGeom.t >= 0 )
      {
        float surfaceAlpha = 1;

        [branch]
        if ( IsOpaqueMaterial( hitGeom.materialIndex ) )
          continue;
      }

      int probeIx = GIProbeIndex( nix, frameParams );
      neighbours += all3DTextures[ prevFrameParams.giSourceIndex ].Load( int4( nix, 0 ) ).rgb * 0.5;
      ncount += 1;
    }

    if ( ncount > 0 )
      neighbours /= ncount;

    destination[ wix ] = float4( avgValue + neighbours, 1 );
  }
}