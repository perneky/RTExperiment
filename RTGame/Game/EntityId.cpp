#include "EntityId.h"
#include "GameStage.h"

EntityId::EntityId( GameStage& stage, uint32_t id )
  : stage( &stage )
  , id( id )
{
}

EntityId::EntityId()
  : stage( nullptr )
  , id( 0 )
{
}

Entity* EntityId::GetEntity() const
{
  if ( id == 0 )
    return nullptr;

  auto iter = stage->entities.find( id );
  return iter == stage->entities.end() ? nullptr : iter->second.get();
}

uint32_t EntityId::GetId() const
{
  return id;
}

EntityId::operator bool() const
{
  return !!id;
}

bool EntityId::operator==( const EntityId& other ) const
{
  return id == other.id;
}

bool EntityId::operator!=( const EntityId& other ) const
{
  return id != other.id;
}
