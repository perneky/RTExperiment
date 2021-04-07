#pragma once

struct CommandList;
struct Resource;

struct Upscaling
{
  enum class Quality
  {
    Off,
    UltraQuality,
    Quality,
    Balanced,
    Performance,
    UltraPerformance,
  };

  static constexpr Quality DefaultQuality = Quality::Off;

  virtual ~Upscaling() = default;

  virtual void Initialize( CommandList& commandList, Quality quality, int outputWidth, int outputHeight ) = 0;
  virtual void TearDown( CommandList& commandList ) = 0;

  virtual XMINT2 GetRenderingResolution() const = 0;
  
  virtual XMFLOAT2 GetJitter() const = 0;

  virtual void Upscale( CommandList& commandList
                      , Resource& lowResColor
                      , Resource& lowResDepth
                      , Resource& lowResMotionVectors
                      , Resource& highResColor
                      , Resource& exposure
                      , float prevExposure
                      , const XMFLOAT2& jitter ) = 0;

  static std::unique_ptr< Upscaling > Instantiate();
};