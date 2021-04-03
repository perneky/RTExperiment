#include "ShaderStructures.hlsli"
#include "../../../ShaderValues.h"

#define _RootSignature "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS )," \
                       "RootConstants( b0, num32BitConstants = 1 )," \
                       "CBV( b1 )," \
                       "CBV( b2, space = 2 )," \
                       "DescriptorTable( SRV( t1, offset = " VaryingResourceBaseSlotStr ", numDescriptors = " VaryingResourceCountStr ", flags = DESCRIPTORS_VOLATILE, space = 1 )," \
                       "                 SRV( t2, offset = " AllMeshParamsSlotStr ",       numDescriptors = 1,                           flags = DESCRIPTORS_VOLATILE, space = 2 ) )," \
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
                       "               filter = FILTER_MIN_MAG_MIP_POINT," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )" \
                       ""

ConstantBuffer< AllMaterialsCB > allMaterials  : register( b2, space2 );
StructuredBuffer< MeshParamsCB > allMeshParams : register( t2, space2 );

Texture2D allMaterialTextures[] : register( t1, space1 );

SamplerState wrapSampler       : register( s0 );
SamplerState clampSampler      : register( s1 );
SamplerState pointClampSampler : register( s2 );

cbuffer ParamIndex : register( b0 )
{
  uint paramIndex;
};

cbuffer FrameParams : register( b1 )
{
  FrameParamsCB frameParams;
};

struct VertexInput
{
  float4 position   : POSITION;
  float2 texcoord   : TEXCOORD;
  float3 normal     : NORMAL;
};

struct VertexOutput
{
  float2 texcoord    : TEXCOORD;
  float3 worldNormal : WORLD_NORMAL;
  float4 screenPosition : SV_POSITION;
};
