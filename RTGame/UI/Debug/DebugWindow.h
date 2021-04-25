#pragma once

#include "../UIWindow.h"
#include "Render/Upscaling.h"

enum class FrameDebugModeCB : int;

class DebugWindow : public UIWindow
{
public:
  DebugWindow();
  ~DebugWindow();

  void Tick( CommandList& commandList, double timeElapsed ) override;

  bool               GetShowGIProbes          () const;
  bool               GetShowLuminanceHistogram() const;
  FrameDebugModeCB   GetFrameDebugMode        () const;
  Upscaling::Quality GetUpscalingQuality      () const;

private:
  double   fpsAccum        = 0;
  uint64_t frameCounter    = 0;
  int      framesPerSecond = 0;
  
  bool             showGIProbes    = false;
  bool             showLuminanceHg = false;
  FrameDebugModeCB frameDebugMode;

  Upscaling::Quality upscalingQuality = Upscaling::DefaultQuality;
};