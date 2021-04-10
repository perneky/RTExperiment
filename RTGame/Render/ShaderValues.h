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
#define VaryingResourceCount    1000

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
#define MaxInstanceCount 1000
#define MaxMaterials     500
#define SceneMaxLights   50

#define CBVHeapSize ( AllResourceCount + MaxMeshCount * 2 )

#define CBVBaseSlot   AllResourceCount
#define CBVIBBaseSlot AllResourceCount
#define CBVVBBaseSlot ( CBVIBBaseSlot + MaxMeshCount )

#define RTSceneCountStr            "100"
#define Engine2DResourceCountStr   "40"
#define EngineCubeResourceCountStr "10"
#define EngineVolResourceCountStr  "10"
#define VaryingResourceCountStr    "1000"

#define RTSceneBaseSlotStr            "0"
#define Engine2DResourceBaseSlotStr   "100"
#define EngineCubeResourceBaseSlotStr "140"
#define EngineVolResourceBaseSlotStr  "150"
#define VaryingResourceBaseSlotStr    "160"

#define CBVIBBaseSlotStr "1160"
#define CBVVBBaseSlotStr "1660"

#define CBVIBCountStr "500"
#define CBVVBCountStr "500"

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
  IndirectLightingSlot,
  DepthSlot0,
  DepthSlot1,
  DepthHQSlot,
  AllMeshParamsSlot,
  ReflectionSlot,
  ReflectionUAVSlot,
  ReflectionProc1Slot,
  ReflectionProc1UAVSlot,
  ReflectionProc2Slot,
  ReflectionProc2UAVSlot,
  SDRUAVSlot,
  HDRUAVSlot,
  HDRHQSlot,
  HDRHQUAVSlot,
  HDRLowLevelUAVSlot,
  ExposureBufferSlot,
  ExposureBufferUAVSlot,
  SpecBRDFLUTSlot,
  SpecBRDFLUTUAVSlot,
  WetnessSlot,
  BloomSlot,
  BloomBlurredSlot,
  BloomUAVSlot,
  BloomBlurredUAVSlot,
  MotionVectorsSlot,

  TextureSlotCount
};

#ifdef __cplusplus
  static_assert( TextureSlotCount < Engine2DResourceBaseSlot + Engine2DResourceCount, "Too many engine textures!" );
#endif // __cplusplus

#define AllMeshParamsSlotStr "108"

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
#define ReflectionProcessDataSizeStr  "52"

#define ExtractBloomKernelWidth  8
#define ExtractBloomKernelHeight 8

#define BlurBloomKernelWidth  8
#define BlurBloomKernelHeight 8

#define LinearUSKernelWidth  8
#define LinearUSKernelHeight 8

#define SpecBRDFLUTSize 256

#define RTSlotMask ( ( 1 << 12 ) - 1 )

#define HaltonSequenceLength 32

#define ENABLE_RAYTRACING_FOR_RENDER 1

#endif // SHADER_VARS_H
