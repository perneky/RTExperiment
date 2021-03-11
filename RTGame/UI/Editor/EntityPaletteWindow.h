#pragma once

#include "../UIWindow.h"
#include "Common/Signal.h"

class GameStage;

class EntityPaletteWindow : public UIWindow
{
public:
  EntityPaletteWindow();
  ~EntityPaletteWindow();

  void Tick( CommandList& commandList, double timeElapsed ) override;

  Signal<>       SigRemoveSelectedEntity;
  Signal<>       SigPlacePointLight;
  Signal<>       SigPlaceSpotLight;
  Signal< GUID > SigPlaceModel;

private:
  std::map< GUID, std::string > entitiesByGUID;
  std::map< std::string, GUID > entitiesByName;

  char filter[ 128 ]      = { 0 };
  bool pointLightSelected = false;
  bool spotLightSelected  = false;
  GUID entitySelected     = {};
};