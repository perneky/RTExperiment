#pragma once

#include "Types.h"
#include "ShaderStructures.h"

struct Window;
struct Factory;
struct Adapter;
struct Device;
struct Swapchain;
struct Resource;
struct CommandList;
struct CommandAllocator;
struct PipelineState;
struct DescriptorHeap;
struct SpriteVertexFormat;
struct ComputeShader;
struct RTTopLevelAccelerator;

class CommandQueueManager;
class AnimationSet;

class RenderManager
{
public:
  static constexpr PixelFormat SDRFormat          = PixelFormat::RGBA1010102UN;
  static constexpr PixelFormat HDRFormat          = PixelFormat::RGBA16161616F;
  static constexpr PixelFormat HDRFormat2         = PixelFormat::RG1616F;
  static constexpr PixelFormat DepthFormat        = PixelFormat::D32;
  static constexpr PixelFormat ReflDepthFormat    = PixelFormat::R32F;
  static constexpr PixelFormat AOFormat           = PixelFormat::RG1616F;
  static constexpr PixelFormat MotionVectorFormat = PixelFormat::RG1616F;
  static constexpr PixelFormat LumaFormat         = PixelFormat::R8U;
  static constexpr PixelFormat HQSFormat          = PixelFormat::R8UN;

  static bool           CreateInstance( std::shared_ptr< Window > window );
  static RenderManager& GetInstance();
  static void           DeleteInstance();

  enum class Sprites
  {
    Light,
  };

  void IdleGPU();

  void SetUp( CommandList& commandList );

  CommandAllocator* RequestCommandAllocator( CommandQueueType queueType );
  void DiscardCommandAllocator( CommandQueueType queueType, CommandAllocator* allocator, uint64_t fenceValue );
  std::unique_ptr< CommandList > CreateCommandList( CommandAllocator* allocator, CommandQueueType queueType );

  void PrepareNextSwapchainBuffer( uint64_t fenceValue );
  uint64_t Submit( std::unique_ptr< CommandList > commandList, CommandQueueType queueType, bool wait );
  uint64_t Present( uint64_t fenceValue );

  int ReserveRTIndexBufferSlot();
  void FreeRTIndexBufferSlot( int slot );

  int ReserveRTVertexBufferSlot();
  void FreeRTVertexBufferSlot( int slot );

  Device&         GetDevice();
  Swapchain&      GetSwapchain();
  PipelineState&  GetPipelinePreset( PipelinePresets preset );
  DescriptorHeap& GetShaderResourceHeap();
  DescriptorHeap& GetSamplerHeap();

  void PrepareAllMaterials( CommandList& commandList );
  int GetAllMaterialIndex( const GUID& guid ) const;
  void BindAllMaterials( CommandList& commandList, int slot );
  void BindAllMaterialsToCompute( CommandList& commandList, int slot );

  ComputeShader& GetSkinTransformShader();
  Resource&      GetSkinTransformBuffer();

  void TidyUp();

  std::unique_ptr< Resource > GetUploadConstantBufferForResource( Resource& resource );
  void ReuseUploadConstantBuffer( std::unique_ptr< Resource > buffer );

  void RecreateWindowSizeDependantResources( CommandList& commandList );

  void BeginSpriteRendering( CommandList& commandList );
  void FinishSpriteRendering( CommandList& commandList, FXMMATRIX viewTransform, CXMMATRIX projTransform );
  void AddSprite( CommandList& commandList, FXMVECTOR position, Sprites type );

  std::shared_ptr< AnimationSet > LoadAnimation( const wchar_t* path, std::function< void( AnimationSet* ) > filler );

  int AddBLASGPUInfo( const BLASGPUInfo& info );
  void FreeBLASGPUInfo( int index );
  Resource& GetBLASGPUInfoResource( CommandList& commandList );

private:
  enum { MaxSpriteVertices = 1000 };

  RenderManager( std::shared_ptr< Window > window );
  ~RenderManager();

  int Load2DTexture( CommandList& commandList, const std::wstring& path );
  int DoLoad2DTexture( CommandList& commandList, const std::wstring& path );

  int LoadCubeTexture( CommandList& commandList, const std::wstring& path );
  int DoLoadCubeTexture( CommandList& commandList, const std::wstring& path );

  static RenderManager* instance;
  
  std::shared_ptr< Window > window;

  std::unique_ptr< Factory >   factory;
  std::unique_ptr< Adapter >   adapter;
  std::unique_ptr< Device >    device;
  std::unique_ptr< Swapchain > swapchain;

  std::unique_ptr< CommandQueueManager > commandQueueManager;

  std::unique_ptr< PipelineState > pipelinePresets[ int( PipelinePresets::PresentCount ) ];

  std::map< CommandQueueType, std::map< uint64_t, std::vector< std::unique_ptr< Resource > > > >              stagingResources;
  std::map< CommandQueueType, std::map< uint64_t, std::vector< std::unique_ptr< RTTopLevelAccelerator > > > > stagingTLAS;
  std::map< CommandQueueType, std::map< uint64_t, std::vector< std::function< void() > > > >                  endFrameCallbacks;

  std::unique_ptr< Resource >                                 allMaterialsConstantBuffer;
  std::array< std::unique_ptr< Resource >, AllResourceCount > allTextures;

  std::unique_ptr< Resource > randomTexture;

  std::unique_ptr< Resource > spriteVertexBuffer;

  std::unique_ptr< Resource >      skinTransformBuffer;
  std::unique_ptr< ComputeShader > skinTransformShader;

  std::map< std::wstring, int > allTextureMap;
  std::map< GUID, int >         allMaterialMap;

  std::mutex         freeRTIndexBufferSlotsLock;
  std::vector< int > freeRTIndexBufferSlots;

  std::mutex         freeRTVertexBufferSlotsLock;
  std::vector< int > freeRTVertexBufferSlots;

  std::mutex                                                  uploadConstantBuffersLock;
  std::map< int, std::vector< std::unique_ptr< Resource > > > uploadConstantBuffers;

  std::mutex                                              animationMapLock;
  std::map< std::wstring, std::weak_ptr< AnimationSet > > animationMap;

  std::vector< SpriteVertexFormat > spriteVertices;

  std::set< int >             freeBLASGPUInfoSlots;
  BLASGPUInfo                 blasGPUInfoList[ MaxMeshCount ];
  std::unique_ptr< Resource > blasGPUInfoBuffer;
  bool                        hasPendingBLASGPUInfoChange = false;

  int spriteTextures[ 1 ];
};
