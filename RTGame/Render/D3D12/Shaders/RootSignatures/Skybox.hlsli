#include "ShaderStructures.hlsli"
#include "../../../ShaderValues.h"

#define _RootSignature "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS )," \
                       "RootConstants( b0, num32BitConstants = 1 )," \
                       "CBV( b1 )," \
                       "CBV( b2, space = 2 )," \
                       "DescriptorTable( SRV( t0, offset = " EngineCubeResourceBaseSlotStr ", numDescriptors = " EngineCubeResourceCountStr ", flags = DESCRIPTORS_VOLATILE, space = 0 )," \
                       "                 SRV( t1, offset = " AllMeshParamsSlotStr ",          numDescriptors = 1,                              flags = DESCRIPTORS_VOLATILE, space = 1 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_WRAP," \
                       "               addressV = TEXTURE_ADDRESS_WRAP," \
                       "               addressW = TEXTURE_ADDRESS_WRAP )" \

cbuffer ParamIndex : register( b0 )
{
  uint paramIndex;
};

cbuffer FrameParams : register( b1 )
{
  FrameParamsCB frameParams;
};

ConstantBuffer< AllMaterialsCB > allMaterials  : register( b2, space2 );
StructuredBuffer< MeshParamsCB > allMeshParams : register( t1, space1 );

TextureCube  allCubeTextures[]  : register( t0, space0 );
SamplerState skySampler         : register( s0 );

struct VertexInput
{
  float4 position : POSITION;
};

struct VertexOutput
{
  float3 texcoord : TEXCOORD;
  float4 screenPosition : SV_POSITION;
};
