#pragma once

#include "../UIWindow.h"
#include "Common/Signal.h"

class GameStage;

class StageWindow : public UIWindow
{
public:
  StageWindow();
  ~StageWindow();

  void SetStage( GameStage* stage );

  void Tick( CommandList& commandList, double timeElapsed ) override;
  
private:
  struct Sky;

  GameStage* stage = nullptr;

  std::vector< Sky > skies;

  int selectedSky = -1;
};