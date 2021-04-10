#pragma once

#include "Types.h"
#include "ShaderStructuresNative.h"
#include "Game/GameDefinition.h"
#include "ShaderStructuresNative.h"
#include "Upscaling.h"

class Mesh;
class Gizmo;
class MeasureCPUTime;
struct Camera;
struct RTTopLevelAccelerator;
struct CommandList;
struct Resource;
struct ResourceDescriptor;
struct ComputeShader;
struct Window;
struct Upscaling;

enum class GizmoType;

class Scene
{
public:
  struct EditorInfo
  {
    std::set< uint32_t > selection;
    GizmoType            gizmoType;
    XMFLOAT4X4           gizmoTransform;
    int                  selectedGizmoElement;
    FrameDebugModeCB     frameDebugMode;
    bool                 renderLightMarkers;
  };

  Scene();
  ~Scene();

  void SetUp( CommandList& commandList, Window& window );

  void SetSky( CommandList& commandList, const GUID& guid );
  
  uint32_t AddMesh( CommandList& commandList, const wchar_t* filePath, bool forceReload, FXMMATRIX transform, std::set< int > subsets, MaterialTranslator materialTranslator );
  uint32_t AddMesh( std::shared_ptr< Mesh > mesh, FXMMATRIX transform, std::set< int > subsets );

  Mesh* GetMesh( uint32_t id );

  void SetVertexBufferForMesh( uint32_t id, Resource* vertexBuffer );
  void SetRayTracingForMesh( uint32_t id, int subset, RTBottomLevelAccelerator* rtAccelerator );

  void SetDirectionalLight( FXMVECTOR direction, float distance, float radius, FXMVECTOR color, float intensity, bool castShadow );
  
  void RemoveMesh( uint32_t id );

  void MoveMesh( uint32_t id, FXMMATRIX worldTransform );
  
  void SetCamera( Camera& camera );

  void RebuildLightCB( CommandList& commandList, const std::vector< LightCB >& lights );
  void UpdateRaytracing( CommandList& commandList );

  std::pair< Resource&, Resource& > Render( CommandList& commandList, const EditorInfo& editorInfo, float dt );

  void SetPostEffectParams( float exposure, float bloomThreshold, float bloomStrength );

  int GetTargetWidth () const;
  int GetTargetHeight() const;

  void RecreateWindowSizeDependantResources( CommandList& commandList, Window& window );

  BoundingBox CalcModelAABB( uint32_t id ) const;

  float Pick( uint32_t id, FXMVECTOR rayStart, FXMVECTOR rayDir ) const;

  void ShowGIProbes( bool show );

  Gizmo* GetGizmo() const;

  void SetupWetness( CommandList& commandList, const XMINT2& origin, const XMINT2& size, int density, const std::vector< uint8_t >& values );

  void SetUpscalingQuality( CommandList& commandList, Window& window, Upscaling::Quality quality );

  void SetGIArea( const BoundingBox& area );

private:
  void RefreshGIProbeInstances( CommandList& commandList );

  void CreateBRDFLUTTexture( CommandList& commandList );

  struct ModelEntry
  {
    ModelEntry() = default;
    ModelEntry( std::shared_ptr< Mesh > mesh, FXMMATRIX transform, std::set< int > subsets );

    BoundingBox         CalcBoundingBox() const;
    BoundingOrientedBox CalcBoundingOrientedBox() const;

    void SetTransform( FXMMATRIX transform );

    std::shared_ptr< Mesh > mesh;
    std::set< int >         subsets;
    XMFLOAT4X4              prevNodeTransform;
    XMFLOAT4X4              nodeTransform;
    XMFLOAT4                randomValues;

    Resource* vertexBufferOverride = nullptr;

    std::map< int, RTBottomLevelAccelerator* > rtAccelerators;
  };

  LightCB directionalLight;

  std::atomic_uint32_t             entryCounter = 0;
  std::map< uint32_t, ModelEntry > modelEntries;

  std::map< std::wstring, std::shared_ptr< Mesh > > loadedMeshes;

  FrameParamsCB prevFrameParams;
  FrameParamsCB frameParams;

  std::unique_ptr< Resource > depthTextures[ 2 ];
  std::unique_ptr< Resource > motionVectorTexture;
  std::unique_ptr< Resource > directLightingTextures[ 2 ];
  std::unique_ptr< Resource > indirectLightingTexture;
  std::unique_ptr< Resource > reflectionTexture;
  std::unique_ptr< Resource > reflectionProcTextures[ 2 ];
  std::unique_ptr< Resource > lowResHDRTexture;
  std::unique_ptr< Resource > highResHDRTexture;
  std::unique_ptr< Resource > sdrTexture;
  std::unique_ptr< Resource > exposureBuffer;
  std::unique_ptr< Resource > giProbeTextures[ 2 ];
  std::unique_ptr< Resource > specBRDFLUTTexture;
  std::unique_ptr< Resource > wetnessTexture;
  std::unique_ptr< Resource > bloomTexture;
  std::unique_ptr< Resource > bloomBlurredTexture;

  std::unique_ptr< Resource > frameConstantBuffer;
  std::unique_ptr< Resource > prevFrameConstantBuffer;
  std::unique_ptr< Resource > lightingConstantBuffer;
  std::unique_ptr< Resource > haltonSequenceBuffer;

  std::unique_ptr< Resource > giProbeVB;
  std::unique_ptr< Resource > giProbeIB;
  int                         giProbeIndexCount = 0;

  std::unique_ptr< Resource > allMeshParamsBuffer;

  std::unique_ptr< ResourceDescriptor > hdrTextureLowLevel;

  std::unique_ptr< ComputeShader > toneMappingShader;
  std::unique_ptr< ComputeShader > downsampleShader;
  std::unique_ptr< ComputeShader > blurShader;
  std::unique_ptr< ComputeShader > combineLightingShader;
  std::unique_ptr< ComputeShader > traceGIProbeShader;
  std::unique_ptr< ComputeShader > adaptExposureShader;
  std::unique_ptr< ComputeShader > specBRDFLUTShader;
  std::unique_ptr< ComputeShader > processReflectionShader;
  std::unique_ptr< ComputeShader > extractBloomShader;
  std::unique_ptr< ComputeShader > blurBloomShader;

  std::unique_ptr< Upscaling > upscaling;

  std::unique_ptr< RTTopLevelAccelerator > rtScene;
  std::unique_ptr< ResourceDescriptor >    rtDescriptor;

  std::unique_ptr< Mesh > skybox;
  int                     skyMaterial = -1;

  std::unique_ptr< Gizmo > gizmo;

  bool drawGIProbes          = false;
  bool giProbeInstancesDirty = true;
  int  giProbeUpdatePerFrame = 1000;
  int  giProbeUpdateNext     = 0;
  int  currentGISource       = 0;

  double giProbeUpdatePerFrameTimeBudget = 0.002;

  int currentTargetIndex = 0;

  float exposure       = 1;
  float bloomThreshold = 4.0f;
  float bloomStrength  = 0.1f;

  float sceneTime = 0;

  LightingEnvironmentParamsCB lightingEnvironmentParams;

  enum class RTState
  {
    Ready,
    TrianglesModified,
    ElementsModified,
  };

  RTState rtState = RTState::ElementsModified;

  BoundingBox sceneAABB;
  BoundingBox giArea;

  std::unique_ptr< MeasureCPUTime > giTimer;

  Upscaling::Quality upscalingQuality = Upscaling::DefaultQuality;

  std::vector< MeshParamsCB > allMeshParams;
};