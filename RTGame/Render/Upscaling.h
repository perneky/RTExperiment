#pragma once

struct CommandList;
struct Resource;

struct Upscaling
{
  enum class Quality
  {
    Quality,
    Balanced,
    Performance,
    UltraPerformance,
  };

  virtual ~Upscaling() = default;

  virtual void Initialize( Quality quality, CommandList& commandList ) = 0;
  virtual void TearDown( CommandList& commandList ) = 0;

  virtual XMINT2 CalcLowResolution( int highResolutionWidth, int highResolutionHeight ) const = 0;

  virtual void Upscale( CommandList& commandList, Resource& lowResColor, Resource& lowResDepth, Resource& lowResMotionVectors, Resource& highResColor ) = 0;

  static std::unique_ptr< Upscaling > Instantiate();
};