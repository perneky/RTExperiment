#pragma once

#include "EntityId.h"

namespace tinyxml2 { class XMLNode; }

class Entity;
class Scene;
class GameDefinition;
class EntityId;
class StageWindow;
struct CommandList;
struct Resource;
struct Window;
enum class LightTypeCB : int;
enum class FrameDebugModeCB : int;
enum class GizmoType;
enum GizmoElement;

class GameStage
{
  friend class EntityId;
  friend class StageWindow;

public:
  struct EditorInfo
  {
    FrameDebugModeCB frameDebugMode;
    GizmoType        activeGizmo;
    bool             renderLightMarkers;
    bool             showGIProbes;
    bool             showLuminanceHistogram;
  };

  GameStage( CommandList& commandList, Window& window );
  ~GameStage();

  void TearDown( CommandList& commandList );

  void SetSky( CommandList& commandList, const GUID& guid );

  EntityId AddModelEntity( CommandList& commandList, const GUID& guid );
  EntityId AddCameraEntity();
  EntityId AddLightEntity( LightTypeCB type );

  EntityId Duplicate( CommandList& commandList, EntityId id );

  void RemoveEntity( EntityId id );

  std::pair< Resource&, Resource& > Render( CommandList& commandList, EntityId camera, float dt, const EditorInfo& editorInfo );

  bool Intersect( int x, int y, XMFLOAT3& hitPoint );

  void Interact( int x, int y, bool activate );

  void SetSelected( EntityId id );

  EntityId GetSelected() const;
  EntityId GetLastUsedCameraEntityId() const;

  void RecreateWindowSizeDependantResources( CommandList& commandList, Window& window );

  void Serialize( tinyxml2::XMLNode& node );
  void Deserialize( CommandList& commandList, tinyxml2::XMLNode& node );

  void Update( CommandList& commandList, double timeElapsed );

  GizmoElement GetActiveGizmoElement() const;
  bool         GetDraggingGizmo() const;
  void         ReleaseGizmo();

  const XMINT2&          GetWetnessOrigin () const;
  const XMINT2&          GetWetnessSize   () const;
  int                    GetWetnessDensity() const;
  std::vector< uint8_t > GetWetnessMap    () const;

  void ChangeWetnessArea( CommandList& commandList, int originX, int originZ, int sizeX, int sizeZ, int density );
  void PaintWetness ( CommandList& commandList, float cx, float cz, float strength, float radius, float exponent );

  Scene* GetScene();

  const BoundingBox& GetGIArea() const;
  void SetGIArea( const BoundingBox& area );

private:
  GUID  skyGUID;

  bool  enableAdaptation = true;
  float targetLuminance  = 0.08f;
  float exposure         = 2;
  float adaptationRate   = 0.05f;
  float minExposure      = 1.0f / 64;
  float maxExposure      = 64;

  float bloomThreshold = 4.0f;
  float bloomStrength  = 0.1f;

  std::atomic_uint32_t                            entityIdGen = 1;
  std::map< uint32_t, std::unique_ptr< Entity > > entities;

  std::unique_ptr< Scene > scene;

  EntityId     selected;
  GizmoElement activeGizmoElement;
  bool         draggingGizmo = false;
  XMINT2       draggingGizmoOrigin;

  EntityId lastUsedCamera;

  XMINT2 wetnessOrigin  = XMINT2(  0,  0 );
  XMINT2 wetnessSize    = XMINT2( 10, 10 );
  int    wetnessDensity = 5;

  std::vector< float > wetness;

  BoundingBox giArea;
};