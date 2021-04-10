#include "ShaderStructures.hlsli"
#include "../../../ShaderValues.h"

#define _RootSignature "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS )," \
                       "RootConstants( b0, num32BitConstants = 1 )," \
                       "CBV( b1 )," \
                       "CBV( b2 )," \
                       "CBV( b3 )," \
                       "DescriptorTable( SRV( t1, offset = " VaryingResourceBaseSlotStr ", numDescriptors = " VaryingResourceCountStr ", flags = DESCRIPTORS_VOLATILE, space = 1 )," \
                       "                 SRV( t2, offset = " AllMeshParamsSlotStr ",       numDescriptors = 1,                           flags = DESCRIPTORS_VOLATILE, space = 2 ) )," \
                       "DescriptorTable( Sampler( s0, numDescriptors = 2 ) )" \
                       ""

ConstantBuffer< AllMaterialsCB > allMaterials  : register( b2 );
StructuredBuffer< MeshParamsCB > allMeshParams : register( t2, space2 );

Texture2D allMaterialTextures[ VaryingResourceCount ] : register( t1, space1 );

SamplerState wrapClampSamplers[] : register( s0 );

#define wrapSampler  wrapClampSamplers[ 0 ]
#define clampSampler wrapClampSamplers[ 1 ]

cbuffer ParamIndex : register( b0 )
{
  uint paramIndex;
};

cbuffer FrameParams : register( b1 )
{
  FrameParamsCB frameParams;
};

cbuffer PrevFrameParams : register( b3 )
{
  FrameParamsCB prevFrameParams;
};

struct VertexInput
{
  float4 position   : POSITION;
  float2 texcoord   : TEXCOORD;
  float3 normal     : NORMAL;
};

struct VertexOutput
{
  float2 texcoord       : TEXCOORD;
  float3 worldNormal    : WORLD_NORMAL;
  float4 screenPosition : SV_POSITION;
#ifdef USE_MOTION_VECTORS
  float4 clipPosition     : CLIP_POSITION;
  float4 prevClipPosition : PREV_CLIP_POSITION;
#endif // USE_MOTION_VECTORS
};
