#include "ShaderStructures.hlsli"
#include "../../../ShaderValues.h"

#define _RootSignature "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS )," \
                       "RootConstants( b0, num32BitConstants = 1 )," \
                       "CBV( b1 )," \
                       "CBV( b2 )," \
                       "CBV( b3 )," \
                       "CBV( b4, space = 4 )," \
                       "CBV( b5, space = 5 )," \
                       "DescriptorTable( SRV( t0, offset = " RTSceneBaseSlotStr            ", numDescriptors = " RTSceneCountStr            ", flags = DESCRIPTORS_VOLATILE, space = 0 )," \
                       "                 SRV( t1, offset = " Engine2DResourceBaseSlotStr   ", numDescriptors = " Engine2DResourceCountStr   ", flags = DESCRIPTORS_VOLATILE, space = 1 )," \
                       "                 SRV( t3, offset = " VaryingResourceBaseSlotStr    ", numDescriptors = " VaryingResourceCountStr    ", flags = DESCRIPTORS_VOLATILE, space = 3 )," \
                       "                 SRV( t4, offset = " EngineCubeResourceBaseSlotStr ", numDescriptors = " EngineCubeResourceCountStr ", flags = DESCRIPTORS_VOLATILE, space = 4 )," \
                       "                 SRV( t5, offset = " EngineVolResourceBaseSlotStr  ", numDescriptors = " EngineVolResourceCountStr  ", flags = DESCRIPTORS_VOLATILE, space = 5 )," \
                       "                 SRV( t6, offset = " CBVIBBaseSlotStr              ", numDescriptors = " CBVIBCountStr              ", flags = DESCRIPTORS_VOLATILE, space = 6 )," \
                       "                 SRV( t7, offset = " CBVVBBaseSlotStr              ", numDescriptors = " CBVVBCountStr              ", flags = DESCRIPTORS_VOLATILE, space = 7 )," \
                       "                 SRV( t8, offset = " AllMeshParamsSlotStr          ", numDescriptors = 1,                              flags = DESCRIPTORS_VOLATILE, space = 8 ) )," \
                       "SRV( t9, space = 9, flags = DATA_VOLATILE )," \
                       "DescriptorTable( Sampler( s0, numDescriptors = 2 ) )," \
                       "StaticSampler( s1, space = 1," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_WRAP," \
                       "               addressV = TEXTURE_ADDRESS_WRAP," \
                       "               addressW = TEXTURE_ADDRESS_WRAP )," \
                       "StaticSampler( s2, space = 2," \
                       "               filter = FILTER_MIN_MAG_MIP_POINT," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \
                       ""

cbuffer ParamIndex : register( b0 )
{
  uint paramIndex;
};

cbuffer FrameParams : register( b1 )
{
  FrameParamsCB frameParams;
};

cbuffer PrevFrameParams : register( b2 )
{
  FrameParamsCB prevFrameParams;
};

cbuffer LightingEnvironmentParams : register( b3 )
{
  LightingEnvironmentParamsCB lightingEnvironmentParams;
};

ConstantBuffer< AllMaterialsCB >   allMaterials   : register( b4, space4 );
ConstantBuffer< HaltonSequenceCB > haltonSequence : register( b5, space5 );
StructuredBuffer< MeshParamsCB >   allMeshParams  : register( t8, space8 );

RaytracingAccelerationStructure rayTracingScene : register( t9, space9 );

Texture2D    allEngineTextures[]   : register( t1, space1 );
Texture2D    allMaterialTextures[] : register( t3, space3 );
TextureCube  allCubeTextures[]     : register( t4, space4 );
Texture3D    all3DTextures[]       : register( t5, space5 );
SamplerState wrapClampSamplers[]   : register( s0 );
SamplerState noiseSampler          : register( s1, space1 );
SamplerState pointClampSampler     : register( s2, space2 );

#define wrapSampler  wrapClampSamplers[ 0 ]
#define clampSampler wrapClampSamplers[ 1 ]

ByteAddressBuffer                  meshIndices [ MaxMeshCount ] : register( t6, space6 );
StructuredBuffer< RTVertexFormat > meshVertices[ MaxMeshCount ] : register( t7, space7 );

struct VertexInput
{
  float4 position : POSITION;
#ifdef HAS_OLD_POSITION
  float4 oldPosition : OLD_POSITION;
#endif // HAS_OLD_POSITION
  float3 normal     : NORMAL;
  float3 tangent    : TANGENT;
  float3 bitangent  : BITANGENT;
  float2 texcoord   : TEXCOORD;
};

struct VertexOutput
{
  float3               worldPosition     : WORLD_POSITION;
  float3               worldNormal       : WORLD_NORMAL;
  float3               worldTangent      : WORLD_TANGENT;
  float3               worldBitangent    : WORLD_BITANGENT;
  float2               texcoord          : TEXCOORD;
  noperspective float3 randomValues      : RANDOM_VALUES;
  float2               clipDepth         : VIEW_DEPTH;
  float3               prevWorldPosition : PREV_WORLD_POSITION;
  float4               prevClipPosition  : PREV_CLIP_POSITION;
  float4               screenPosition    : SV_POSITION;
};
