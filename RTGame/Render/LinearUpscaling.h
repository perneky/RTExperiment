#pragma once

#include "Upscaling.h"

struct ComputeShader;

class LinearUpscaling : public Upscaling
{
public:
  ~LinearUpscaling();

  void Initialize( Quality quality, CommandList& commandList ) override;
  void TearDown( CommandList& commandList ) override;

  XMINT2 CalcLowResolution( int highResolutionWidth, int highResolutionHeight ) const override;

  void Upscale( CommandList& commandList, Resource& lowResColor, Resource& lowResDepth, Resource& lowResMotionVectors, Resource& highResColor ) override;

private:
  Quality quality = Quality::Quality;

  std::unique_ptr< ComputeShader > copier;
};