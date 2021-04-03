#pragma once

#include "Entity.h"

class Scene;
class GameStage;
class AnimationPlayer;
struct CommandList;
struct Resource;
struct RTBottomLevelAccelerator;
struct SlotEntry;

using SlotConfig = std::map< GUID, SlotEntry >;

class ModelEntity : public Entity
{
  friend struct ModelEntityHelper;

public:
  static std::unique_ptr< ModelEntity > CreateDeserialize( tinyxml2::XMLNode& node, CommandList& commandList, GameStage& stage, Scene& scene );

  ModelEntity( CommandList& commandList, GameStage& stage, Scene& scene, const GUID& guid, SlotConfig&& slotConfig = {} );
  ~ModelEntity();

  float Pick( FXMVECTOR rayStart, FXMVECTOR rayDir ) const override;

  void Serialize  ( uint32_t id, tinyxml2::XMLNode& node ) override;
  void Deserialize( tinyxml2::XMLNode& node ) override;

  void Update( CommandList& commandList, bool mayUseGPU, double timeElapsed ) override;

  std::unique_ptr< Entity > Duplicate( CommandList& commandList ) override;

  std::vector< const wchar_t* > GetAnimationNames() const;
  const wchar_t* GetCurrentAnimationName() const;
  void SetCurrentAnimationName( const wchar_t* name );

  std::set< uint32_t > GetMeshIds() const;

  GUID GetGUID() const;

  BoundingBox CalcAABB() const;

  const SlotConfig& GetSlotConfig() const;
  void SetSlotConfig( CommandList& commandList, SlotConfig&& config );

protected:
  void TransformChanged() override;

  void AddToScene( CommandList& commandList );
  void RemoveFromScene();

  Scene*     scene;
  GameStage* stage;

  GUID guid;

  std::vector< uint32_t > sceneMeshes;

  std::unique_ptr< AnimationPlayer >          animationPlayer;
  std::unique_ptr< Resource >                 transformedSkinVertices;
  std::unique_ptr< Resource >                 transformedSkinRTVertexBuffer;
  int                                         skinVertexCount = 0;
  
  std::map< int, std::unique_ptr< RTBottomLevelAccelerator > > skinRTAccelerators;

  std::set< int > subsets;

  SlotConfig slotConfig;
};