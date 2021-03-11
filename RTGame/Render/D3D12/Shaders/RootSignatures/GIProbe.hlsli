#include "ShaderStructures.hlsli"
#include "../../../ShaderValues.h"

#define _RootSignature "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS )," \
                       "RootConstants( num32BitConstants=16, b0 )," \
                       "CBV( b1 )," \
                       "DescriptorTable( SRV( t0, offset = " VolTextureBaseSlotStr ", numDescriptors = " VolTextureCountStr ", flags = DESCRIPTORS_VOLATILE, space = 0 ) )," \
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

cbuffer MeshParams : register( b0 )
{
  matrix vpTransform;
};

cbuffer FrameParams : register( b1 )
{
  FrameParamsCB frameParams;
};

Texture3D    all3DTextures[]       : register( t0, space0 );
SamplerState wrapSampler           : register( s0 );
SamplerState clampSampler          : register( s1 );
SamplerState noiseSampler          : register( s2 );
SamplerState pointClampSampler     : register( s3 );

struct VertexInput
{
  float4 position : POSITION;
};

struct VertexOutput
{
  float3 worldPosition    : WORLD_POSITION;
  float3 worldNormal      : WORLD_NORMAL;
  float4 screenPosition   : SV_POSITION;
  
  nointerpolation uint probeId : PROBE_ID;
};
