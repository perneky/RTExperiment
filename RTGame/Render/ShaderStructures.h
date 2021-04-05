#pragma once

#include "ShaderValues.h"

enum class VRSBlock : int
{
  _1x1,
  _1x2,
  _2x1,
  _2x2,
};

enum class FrameDebugModeCB : int
{
  None,
  ShowGI,
  ShowAO,
  ShowGIAO,
  ShowSurfaceNormal,
  ShowGeometryNormal,
  ShowRoughness,
  ShowMetallic,
  ShowBaseReflection,
  ShowProcessedReflection,
};

struct MeshParamsCB
{
  XMFLOAT4X4 worldTransform;
  XMFLOAT4X4 wvpTransform;
  XMFLOAT4X4 wvTransform;
  XMFLOAT4X4 prevWorldTransform;
  XMFLOAT4X4 prevWVPTransform;
  XMFLOAT4X4 prevWVTransform;
  XMFLOAT4   randomValues;
  int        materialIndex;
  float      editorAddition;
  int        padding;
};

struct FrameParamsCB
{
  XMFLOAT4X4       vpTransform;
  XMFLOAT4X4       viewTransform;
  XMFLOAT4X4       projTransform;
  XMFLOAT4X4       invProjTransform;
  XMFLOAT4         cameraPosition;
  XMFLOAT4         cameraDirection;
  XMFLOAT4         screenSizeLQ;
  XMFLOAT4         screenSizeHQ;
  XMFLOAT4         invWorldSize;
  XMFLOAT4         worldMinCorner;
  XMFLOAT4         wetnessOriginInvSize;
  XMFLOAT4         giProbeOrigin;
  XMINT4           giProbeCount;
  int              frameIndex;
  int              depthIndex;
  int              prevDepthIndex;
  int              aoIndex;
  int              prevAOIndex;
  int              giSourceIndex;
  float            wetnessDensity;
  float            globalTime;
  FrameDebugModeCB frameDebugMode;
  XMINT3           padding;
};

enum class LightTypeCB : int
{
  Directional,
  Point,
  Spot,
};

struct LightCB
{
  XMFLOAT4    origin;
  XMFLOAT4    direction;
  XMFLOAT4    color;
  float       sourceRadius;
  float       attenuationStart;
  float       attenuationEnd;
  float       attenuationFalloff;
  float       theta;
  float       phi;
  float       coneFalloff;
  int         castShadow;
  LightTypeCB type;
  XMINT3      padding;
};

struct LightingEnvironmentParamsCB
{
  LightCB  sceneLights[ SceneMaxLights ];
  int      lightCount;
  int      skyMaterial;
};

enum MaterialFeatures : int
{
  FlipNormalX   = 1,
  ScaleUV       = 2,
  ClampToEdge   = 4,
  UseWetness    = 8,
  TwoSided      = 16,
  UseSpecular   = 32,
};

struct MaterialCB
{
  XMINT4   textureIndices; // albedoTex, normalTex, metallicTex, roughnessTex
  XMINT4   args;           // features, emissiveTex, _, alphaMode
  XMFLOAT4 textureValues;  // metallicVal, roughnessVal
};

struct AllMaterialsCB
{
  MaterialCB mat[ MaxMaterials ];
};

struct InstanceParamsCB
{
  XMFLOAT4X4 worldTransform;
};

struct GIProbeInstanceCB
{
  XMFLOAT4 position_texture;
};

enum class AlphaModeCB : int
{
  Opaque,
  OneBitAlpha,
  Translucent,
};

struct RigidVertexFormat
{
  XMHALF4 position;
  XMHALF4 normal;
  XMHALF4 tangent;
  XMHALF4 bitangent;
  XMHALF2 texcoord;
};

struct GIProbeVertexFormat
{
  XMHALF4 position;
};

struct GizmoVertexFormat
{
  XMFLOAT3 position;
  uint32_t color;
};

struct RigidVertexFormatWithHistory
{
  XMHALF4 position;
  XMHALF4 oldPosition;
  XMHALF4 normal;
  XMHALF4 tangent;
  XMHALF4 bitangent;
  XMHALF2 texcoord;
};

struct SkinnedVertexFormat
{
  XMHALF4  position;
  XMHALF4  normal;
  XMHALF4  tangent;
  XMHALF4  bitangent;
  XMHALF2  texcoord;
  uint32_t blendIndices;
  uint32_t blendWeights;
};

struct SkyVertexFormat
{
  XMHALF4 position;
};

struct RTVertexFormat
{
  XMFLOAT3 normal;
  XMFLOAT3 tangent;
  XMFLOAT3 bitangent;
  XMFLOAT2 texcoord;
  uint32_t materialIndex;
};

