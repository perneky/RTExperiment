#pragma once

#ifndef SHADER_VARS_H
#define SHADER_VARS_H

#define PI   3.141592654f
#define PIPI ( 3.141592654f * 2 )

#define GIProbeSpacing 1.0f
#define GIProbeWidth   44
#define GIProbeHeight  22

#define TwoDTextureCount     1000
#define EngineResourceCount  30
#define MaterialTextureCount ( TwoDTextureCount - EngineResourceCount )
#define CubeTextureCount     10
#define VolTextureCount      10
#define AllResourceCount     ( TwoDTextureCount + CubeTextureCount + VolTextureCount )

#define EngineTextureBaseSlot   0
#define MaterialTextureBaseSlot EngineResourceCount
#define CubeTextureBaseSlot     TwoDTextureCount
#define VolTextureBaseSlot      ( CubeTextureBaseSlot + CubeTextureCount )

#define RandomTextureSlotCPP  VolTextureBaseSlot
#define RandomTextureSlotHLSL ( RandomTextureSlotCPP - VolTextureBaseSlot )
#define RandomTextureSize     128
#define RandomTextureScale    0.1

#define GITexture1Slot     ( RandomTextureSlotCPP + 1 )
#define GITexture1SlotHLSL ( GITexture1Slot - VolTextureBaseSlot )
#define GITexture1UAVSlot  ( GITexture1Slot + 1 )

#define GITexture2Slot     ( GITexture1UAVSlot + 1 )
#define GITexture2SlotHLSL ( GITexture2Slot - VolTextureBaseSlot )
#define GITexture2UAVSlot  ( GITexture2Slot + 1 )

#define MaxMeshCount     500
#define MaxInstanceCount 1000
#define MaxMaterials     500
#define SceneMaxLights   50

#define CBVHeapSize ( AllResourceCount + MaxMeshCount * 2 )

#define CBVBaseSlot   AllResourceCount
#define CBVIBBaseSlot AllResourceCount
#define CBVVBBaseSlot ( CBVIBBaseSlot + MaxMeshCount )

#define EngineTextureCountStr   "30"
#define MaterialTextureCountStr "970"
#define CubeTextureCountStr     "10"
#define VolTextureCountStr      "10"

#define EngineTextureBaseSlotStr   "0"
#define MaterialTextureBaseSlotStr "30"
#define CubeTextureBaseSlotStr     "1000"
#define VolTextureBaseSlotStr      "1010"

#define CBVIBBaseSlotStr "1020"
#define CBVVBBaseSlotStr "1520"

#define CBVIBCountStr "500"
#define CBVVBCountStr "500"

enum TextureSlots
{
  SDRSlot,
  HDRSlot,
  DirectLighting1Slot,
  DirectLighting2Slot,
  IndirectLightingSlot,
  DepthSlot0,
  DepthSlot1,
  AllMeshParamsSlot,
  ReflectionSlot,
  ReflectionUAVSlot,
  ReflectionProc1Slot,
  ReflectionProc1UAVSlot,
  ReflectionProc2Slot,
  ReflectionProc2UAVSlot,
  GaussianBufferSlot,
  SDRUAVSlot,
  HDRUAVSlot,
  HDRLowLevelUAVSlot,
  ExposureBufferUAVSlot,
  SpecBRDFLUTSlot,
  SpecBRDFLUTUAVSlot,
  WetnessSlot,
  BloomSlot,
  BloomBlurredSlot,
  BloomUAVSlot,
  BloomBlurredUAVSlot,

  TextureSlotCount
};

#ifdef __cplusplus
  static_assert( TextureSlotCount < 30, "Too many engine textures!" );
#endif // __cplusplus

#define AllMeshParamsSlotStr "7"

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

#define SpecBRDFLUTSize 256

#define RTSlotMask ( ( 1 << 12 ) - 1 )

#define ENABLE_RAYTRACING_FOR_RENDER 1

#endif // SHADER_VARS_H
