#include "ShaderStructures.hlsli"
#include "../../../ShaderValues.h"

#define _RootSignature "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS )," \
                       "CBV( b0 )," \
                       "CBV( b1 )," \
                       "CBV( b2 )," \
                       "CBV( b3 )," \
                       "SRV( t7, space = 7, flags = DATA_VOLATILE )," \
                       "SRV( t0, space = 0, flags = DATA_VOLATILE )," \
                       "DescriptorTable( SRV( t1, offset = " EngineTextureBaseSlotStr   ", numDescriptors = " EngineTextureCountStr   ", flags = DESCRIPTORS_VOLATILE, space = 1 )," \
                       "                 SRV( t2, offset = " MaterialTextureBaseSlotStr ", numDescriptors = " MaterialTextureCountStr ", flags = DESCRIPTORS_VOLATILE, space = 2 )," \
                       "                 SRV( t3, offset = " CubeTextureBaseSlotStr     ", numDescriptors = " CubeTextureCountStr     ", flags = DESCRIPTORS_VOLATILE, space = 3 )," \
                       "                 SRV( t4, offset = " VolTextureBaseSlotStr      ", numDescriptors = " VolTextureCountStr      ", flags = DESCRIPTORS_VOLATILE, space = 4 )," \
                       "                 SRV( t5, offset = " CBVIBBaseSlotStr           ", numDescriptors = " CBVIBCountStr           ", flags = DESCRIPTORS_VOLATILE, space = 5 )," \
                       "                 SRV( t6, offset = " CBVVBBaseSlotStr           ", numDescriptors = " CBVVBCountStr           ", flags = DESCRIPTORS_VOLATILE, space = 6 ) )," \
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
                       "               addressW = TEXTURE_ADDRESS_CLAMP," \
                       "               maxAnisotropy = 16 )" \
                       ""

cbuffer MeshParams : register( b0 )
{
  MeshParamsCB meshParams;
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

StructuredBuffer< MaterialCB > allMaterials : register( t7, space7 );

RaytracingAccelerationStructure rayTracingScene : register( t0, space0 );

Texture2D    allEngineTextures[]   : register( t1, space1 );
Texture2D    allMaterialTextures[] : register( t2, space2 );
TextureCube  allCubeTextures[]     : register( t3, space3 );
Texture3D    all3DTextures[]       : register( t4, space4 );
SamplerState wrapSampler           : register( s0 );
SamplerState clampSampler          : register( s1 );
SamplerState noiseSampler          : register( s2 );
SamplerState pointClampSampler     : register( s3 );

ByteAddressBuffer                  meshIndices[ MaxMeshCount ] : register( t5, space5 );
StructuredBuffer< RTVertexFormat > meshVertices[ MaxMeshCount ] : register( t6, space6 );

struct VertexInput
{
  float4 position  : POSITION;
  float3 normal    : NORMAL;
  float3 tangent   : TANGENT;
  float3 bitangent : BITANGENT;
  float2 texcoord  : TEXCOORD;
};

struct VertexOutput
{
  float3               worldPosition    : WORLD_POSITION;
  float3               worldNormal      : WORLD_NORMAL;
  float3               worldTangent     : TANGENT;
  float3               worldBitangent   : BITANGENT;
  float2               texcoord         : TEXCOORD;
  noperspective float3 randomValues     : RANDOM_VALUES;
  float2               clipDepth        : VIEW_DEPTH;
  float4               prevClipPosition : PREV_CLIP_POSITION;
  float4               screenPosition   : SV_POSITION;
};
