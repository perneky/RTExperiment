#pragma once

#include "Upscaling.h"

struct ComputeShader;
struct NVSDK_NGX_Parameter;
struct NVSDK_NGX_Handle;

class DLSSUpscaling : public Upscaling
{
public:
  ~DLSSUpscaling();

  void Initialize( CommandList& commandList, Quality quality, int outputWidth, int outputHeight ) override;
  void TearDown( CommandList& commandList ) override;

  XMINT2 GetRenderingResolution() const override;

  XMFLOAT2 GetJitter() const override;

  void Upscale( CommandList& commandList
              , Resource& lowResColor
              , Resource& lowResDepth
              , Resource& lowResMotionVectors
              , Resource& highResColor
              , Resource& exposure
              , float prevExposure
              , const XMFLOAT2& jitter ) override;

  static bool IsAvailable();

private:
  Quality quality = Quality::Quality;

  int renderWidth  = 0;
  int renderHeight = 0;

  int renderMinWidth  = 0;
  int renderMaxWidth  = 0;
  int renderMinHeight = 0;
  int renderMaxHeight = 0;

  float sharpness = 1;

  NVSDK_NGX_Parameter* params = nullptr;
  NVSDK_NGX_Handle*    dlss   = nullptr;

  std::vector< XMFLOAT2 > jitterSequence;
  int                     jitterPhase = 0;
};