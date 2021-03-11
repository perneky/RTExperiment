#pragma once

#include "../UIWindow.h"
#include "Common/Signal.h"

struct Resource;
struct CommandList;

class GameStage;

enum class GizmoType;

class ToolWindow : public UIWindow
{
public:
  enum class ActiveTool
  {
    Move,
    Rotate,
    Scale,
    Wetness,
  };

  ToolWindow( CommandList& commandList );

  ActiveTool GetActiveTool() const;
  GizmoType  GetActiveGizmo() const;

  void Tick( CommandList& commandList, double timeElapsed ) override;

  void SetGameStage( GameStage* gameStage );

  void GetWetnessParams( float& brushStrength, float& brushRadius, float& brushExponent ) const;

private:
  ActiveTool activeTool = ActiveTool::Move;

  std::unique_ptr< Resource > moveIcon;
  std::unique_ptr< Resource > rotateIcon;
  std::unique_ptr< Resource > scaleIcon;
  std::unique_ptr< Resource > wetnessIcon;

  GameStage* gameStage = nullptr;

  float wetnessBrushStrength = 0.1f;
  float wetnessBrushRadius   = 5.0f;
  float wetnessBrushExponent = 2.0f;
};