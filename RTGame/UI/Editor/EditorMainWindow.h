#pragma once

#include "../PersistentValue.h"
#include "../UIWindow.h"
#include "Common/Signal.h"

struct Window;
class GameStage;

static const wchar_t Editor_LastLoadedLevel[]   = L"Editor_LastLoadedLevel";
static const wchar_t Editor_AutoLoadLastLevel[] = L"Editor_AutoLoadLastLevel";
static const wchar_t Editor_ShowLightMarkers[]  = L"Editor_ShowLightMarkers";

class EditorMainWindow : public UIWindow
{
public:
  EditorMainWindow( Window& window );
  ~EditorMainWindow();

  void Tick( CommandList& commandList, double timeElapsed ) override;
  
  void SetStage( GameStage* stage, const wchar_t* path );

  bool ShouldShowLightMarkers() const;

  Signal< const wchar_t* > SigStageChangeRequested;

private:
  void SaveStage( const wchar_t* path );

  Window* window = nullptr;
  GameStage* stage = nullptr;
  bool requestInitialLoad = false;

  PersistentValue< std::wstring, Editor_LastLoadedLevel   > openedScenePath;
  PersistentValue< bool,         Editor_AutoLoadLastLevel > autoLoadLastLevel;
  PersistentValue< bool,         Editor_ShowLightMarkers >  showLightMarkers;
};