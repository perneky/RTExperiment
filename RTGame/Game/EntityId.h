#pragma once

#include "Entity.h"

class Entity;

class EntityId
{
  friend class GameStage;

public:
  EntityId();

  Entity*  GetEntity() const;
  uint32_t GetId() const;

  operator bool() const;

  bool operator == ( const EntityId& other ) const;
  bool operator != ( const EntityId& other ) const;

protected:
  EntityId( GameStage& stage, uint32_t id );

  GameStage* stage;
  uint32_t   id;
};