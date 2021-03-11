#include "ShaderStructures.hlsli"
#include "../../../ShaderValues.h"

#define _RootSignature "RootFlags( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | DENY_HULL_SHADER_ROOT_ACCESS | DENY_DOMAIN_SHADER_ROOT_ACCESS | DENY_GEOMETRY_SHADER_ROOT_ACCESS )," \
                       "RootConstants( b0, num32BitConstants = 1 )," \
                       "CBV( b1 )" \
                       ""

cbuffer GizmoParams : register( b0 )
{
  uint selectedGizmo;
}

cbuffer MeshParams : register( b1 )
{
  MeshParamsCB meshParams;
};

struct VertexInput
{
  float4 position : POSITION;
  float4 color    : COLOR;
};

struct VertexOutput
{
  float4 color    : COLOR;
  float4 screenPosition : SV_POSITION;
};
