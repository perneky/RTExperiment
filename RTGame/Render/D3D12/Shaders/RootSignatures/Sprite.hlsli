#include "ShaderStructures.hlsli"
#include "../../../ShaderValues.h"

#define _RootSignature "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS )," \
                       "RootConstants( b0, num32BitConstants = 32 )," \
                       "DescriptorTable( SRV( t0, offset = " MaterialTextureBaseSlotStr ", numDescriptors = " MaterialTextureCountStr ", flags = DESCRIPTORS_VOLATILE, space = 0 ) )," \
                       "StaticSampler( s0," \
                       "               filter = FILTER_MIN_MAG_MIP_LINEAR," \
                       "               addressU = TEXTURE_ADDRESS_CLAMP," \
                       "               addressV = TEXTURE_ADDRESS_CLAMP," \
                       "               addressW = TEXTURE_ADDRESS_CLAMP )," \
                       ""

cbuffer MeshParams : register( b0 )
{
  matrix viewTransform;
  matrix projTransform;
};

Texture2D allMaterialTextures[] : register( t0, space0 );

SamplerState linearSampler : register( s0 );

struct VertexInput
{
  float4 position  : POSITION;
  float2 size      : SIZE;
  float2 texcoord  : TEXCOORD;
  uint   textureId : TEXID;
};

struct VertexOutput
{
  float2 texcoord  : TEXCOORD;
  uint   textureId : TEXID;
  float4 screenPosition : SV_POSITION;
};
