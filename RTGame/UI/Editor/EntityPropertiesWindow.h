#pragma once

#include "../UIWindow.h"
#include "Game/EntityId.h"
#include "Common/Signal.h"

class GameStage;

class EntityPropertiesWindow : public UIWindow
{
public:
  struct SlotEntry
  {
    int modelIx = 0;
    int materialIx = 0;
  };

  EntityPropertiesWindow();
  ~EntityPropertiesWindow();

  void SetEntity( EntityId id );

  void Tick( CommandList& commandList, double timeElapsed ) override;

  Signal< EntityId >                                    SigDuplicateEntity;
  Signal< EntityId, XMFLOAT3, XMFLOAT3, XMFLOAT3, int > SigMakeArrayOfEntities;
  Signal< EntityId, std::vector< SlotEntry > >          SigSlotSetupChanged;

private:
  void HandleSlotEntity();

  EntityId id;

  bool skipChange = false;
  bool makeArray  = false;

  XMFLOAT3 arrayPosOffset;
  XMFLOAT3 arrayRotOffset;
  XMFLOAT3 arraySizOffset;
  int      arrayCount;

  std::vector< SlotEntry > slotConfig;
};