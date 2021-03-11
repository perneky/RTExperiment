#pragma once

#include "../UIWindow.h"

enum class FrameDebugModeCB : int;

class DebugWindow : public UIWindow
{
public:
  DebugWindow();
  ~DebugWindow();

  void Tick( CommandList& commandList, double timeElapsed ) override;

  bool             GetShowGIProbes  () const;
  FrameDebugModeCB GetFrameDebugMode() const;

private:
  double   fpsAccum        = 0;
  uint64_t frameCounter    = 0;
  int      framesPerSecond = 0;
  
  bool             showGIProbes = false;
  FrameDebugModeCB frameDebugMode;
};