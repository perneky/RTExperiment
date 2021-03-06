#pragma once

#ifndef SHADER_VARS_H
#define SHADER_VARS_H

#define PI   3.141592654f
#define PIPI ( 3.141592654f * 2 )

#define GIProbeSpacing 1.0f
#define GIProbeWidth   44
#define GIProbeHeight  22

#define RTSceneCount            100
#define Engine2DResourceCount   40
#define EngineCubeResourceCount 10
#define EngineVolResourceCount  10
#define VaryingResourceCount    2000

#define AllResourceCount ( RTSceneCount + Engine2DResourceCount + EngineCubeResourceCount + EngineVolResourceCount + VaryingResourceCount )

#define RTSceneBaseSlot            0
#define Engine2DResourceBaseSlot   ( RTSceneBaseSlot            + RTSceneCount )
#define EngineCubeResourceBaseSlot ( Engine2DResourceBaseSlot   + Engine2DResourceCount )
#define EngineVolResourceBaseSlot  ( EngineCubeResourceBaseSlot + EngineCubeResourceCount )
#define VaryingResourceBaseSlot    ( EngineVolResourceBaseSlot  + EngineVolResourceCount )

#define RandomTextureSlotCPP  EngineVolResourceBaseSlot
#define RandomTextureSlotHLSL ( RandomTextureSlotCPP - EngineVolResourceBaseSlot )
#define RandomTextureSize     128
#define RandomTextureScale    0.1

#define GITexture1Slot     ( RandomTextureSlotCPP + 1 )
#define GITexture1SlotHLSL ( GITexture1Slot - EngineVolResourceBaseSlot )
#define GITexture1UAVSlot  ( GITexture1Slot + 1 )

#define GITexture2Slot     ( GITexture1UAVSlot + 1 )
#define GITexture2SlotHLSL ( GITexture2Slot - EngineVolResourceBaseSlot )
#define GITexture2UAVSlot  ( GITexture2Slot + 1 )

#define MaxMeshCount     500
#define MaxMeshWHCount   500
#define MaxInstanceCount 1000
#define MaxMaterials     500
#define SceneMaxLights   50

#define CBVHeapSize ( AllResourceCount + MaxMeshCount * 3 )

#define CBVBaseSlot     AllResourceCount
#define CBVIBBaseSlot   AllResourceCount
#define CBVVBBaseSlot   ( CBVIBBaseSlot + MaxMeshCount )
#define CBVVBWHBaseSlot ( CBVVBBaseSlot + MaxMeshCount )

#define RTSceneCountStr            "100"
#define Engine2DResourceCountStr   "40"
#define EngineCubeResourceCountStr "10"
#define EngineVolResourceCountStr  "10"
#define VaryingResourceCountStr    "2000"

#define RTSceneBaseSlotStr            "0"
#define Engine2DResourceBaseSlotStr   "100"
#define EngineCubeResourceBaseSlotStr "140"
#define EngineVolResourceBaseSlotStr  "150"
#define VaryingResourceBaseSlotStr    "160"

#define CBVIBBaseSlotStr   "2160"
#define CBVVBBaseSlotStr   "2660"
#define CBVVBWHBaseSlotStr "3160"

#define CBVIBCountStr   "500"
#define CBVVBCountStr   "500"
#define CBVVBWHCountStr "500"

enum TextureSlots
{
  BaseSlot
#ifdef __cplusplus
  = Engine2DResourceBaseSlot
#endif // __cplusplus
  ,

  SDRSlot = BaseSlot,
  HDRSlot,
  DirectLighting1Slot,
  DirectLighting2Slot,
  HQS1Slot,
  HQS2Slot,
  IndirectLightingSlot,
  DepthSlot0,
  DepthSlot1,
  DepthHQSlot,
  AllMeshParamsSlot,
  ReflectionSlot,
  ReflectionUAVSlot,
  SDRUAVSlot,
  HDRUAVSlot,
  HDRHQSlot,
  HDRHQUAVSlot,
  SpecBRDFLUTSlot,
  SpecBRDFLUTUAVSlot,
  WetnessSlot,
  MotionVectorsSlot,
  BLASGPUInfoSlot,

  TextureSlotCount
};

#ifdef __cplusplus
  static_assert( TextureSlotCount < Engine2DResourceBaseSlot + Engine2DResourceCount, "Too many engine textures!" );
#endif // __cplusplus

#define AllMeshParamsSlotStr "110"
#define BLASGPUInfoSlotStr   "121"

#define ToneMappingKernelWidth  8
#define ToneMappingKernelHeight 8

#define LightCombinerKernelWidth  8
#define LightCombinerKernelHeight 8

#define DownsamplingKernelWidth  8
#define DownsamplingKernelHeight 8

#define FilterAmbientKernelWidth  8
#define FilterAmbientKernelHeight 8

#define BlurKernelWidth  8
#define BlurKernelHeight 8

#define AmbientKernelWidth  8
#define AmbientKernelHeight 8
#define AmbientKernelDepth  8

#define SkinTransformKernelWidth 64

#define TraceVolumeAmbientKernelWidth  8
#define TraceVolumeAmbientKernelHeight 8
#define TraceVolumeAmbientKernelDepth  1

#define GIProbeKernelWidth  GIProbeWidth
#define GIProbeKernelHeight GIProbeHeight

#define ReflectionProcessKernelWidth  8
#define ReflectionProcessKernelHeight 8
#define ReflectionProcessTaps         16

#define ExtractBloomKernelWidth  8
#define ExtractBloomKernelHeight 8

#define DownsampleBloomKernelWidth  8
#define DownsampleBloomKernelHeight 8

#define UpsampleBlurBloomKernelWidth  8
#define UpsampleBlurBloomKernelHeight 8

#define BlurBloomKernelWidth  8
#define BlurBloomKernelHeight 8

#define LinearUSKernelWidth  8
#define LinearUSKernelHeight 8

#define ExtractLumaKernelWidth  8
#define ExtractLumaKernelHeight 8

#define GenerateHistogramKernelWidth  16
#define GenerateHistogramKernelHeight 16

#define AdaptExposureKernelWidth  256
#define AdaptExposureKernelHeight 1

#define SpecBRDFLUTSize 256

#define RTSlotMask ( ( 1 << 12 ) - 1 )

#define HaltonSequenceLength 511

#define ENABLE_RAYTRACING_FOR_RENDER 1

#define LightClippingThreshold 0.001
#define OcclusionThreshold     0.002

#endif // SHADER_VARS_H
