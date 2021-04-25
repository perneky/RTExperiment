#pragma once

enum class CommandQueueType
{
  Direct,
  Compute,
  Copy,
};

enum class ResourceDescriptorType
{
  Typeless,
  ConstantBufferView,
  ShaderResourceView,
  RenderTargetView,
  DepthStencilView,
  UnorderedAccessView,
};

enum ResourceStateBits
{
  Common                  = 1 << 0,
  VertexOrConstantBuffer  = 1 << 1,
  IndexBuffer             = 1 << 2,
  RenderTarget            = 1 << 3,
  UnorderedAccess         = 1 << 4,
  DepthWrite              = 1 << 5,
  DepthRead               = 1 << 6,
  PixelShaderInput        = 1 << 7,
  NonPixelShaderInput     = 1 << 8,
  StreamOut               = 1 << 9,
  IndirectArgument        = 1 << 10,
  CopySource              = 1 << 11,
  CopyDestination         = 1 << 12,
  ResolveSource           = 1 << 13,
  ResolveDestination      = 1 << 14,
  RTAccelerationStructure = 1 << 15,
  ShadingRateSource       = 1 << 16,
  GenericRead             = 1 << 17,
  Present                 = 1 << 18,
  Predication             = 1 << 19,
};

struct ResourceState 
{ 
  ResourceState() = default;
  ResourceState( ResourceStateBits bit ) : bits( bit ) {}
  ResourceState( int bits ) : bits( bits ) {}
  uint32_t bits = 0;
};

enum class PrimitiveType
{
  PointList,
  LineList,
  LineStrip,
  TriangleList,
  TriangleStrip,
};

enum class PixelFormat
{
  Unknown,

  R8U,
  R8UN,
  A8UN,
  RG88UN,
  RGBA8888UN,
  BGRA8888UN,
  RGBA1010102UN,
  RGBA16161616F,
  RG1616F,
  R16UN,
  R32F,
  BC1UN,
  BC2UN,
  BC3UN,
  BC4UN,
  BC5UN,

  RGBA1010102U,

  D24S8,
  D32,
  D16,
};

inline bool IsDepthFormat( PixelFormat format )
{
  return format == PixelFormat::D16 || format == PixelFormat::D24S8 || format == PixelFormat::D32;
}

enum class BlendPreset
{
  DisableColorWrite,
  DirectWrite,
  Additive,
  Subtractive,
  Min,
  Max,
  AlphaTest,
  Blend,
  AddRGB,
};

struct BlendDesc
{
  enum class BlendOperation
  {
    Add,
    Sub,
    RevSub,
    Min,
    Max,
  };

  enum class BlendMode
  {
    Zero,
    One,
    SrcColor,
    InvSrcColor,
    SrcAlpha,
    InvSrcAlpha,
    DestColor,
    InvDestColor,
    DestAlpha,
    InvDestAlpha,
  };

  BlendDesc() = default;
  BlendDesc( BlendPreset mode )
  {
    switch ( mode )
    {
    case BlendPreset::DisableColorWrite:
      colorWrite = false;
      break;

    case BlendPreset::DirectWrite:
      break;

    case BlendPreset::Additive:
      alphaBlend            = true;
      colorBlendOperation   = BlendOperation::Add;
      sourceColorBlend      = BlendMode::SrcAlpha;
      destinationColorBlend = BlendMode::One;
      break;

    case BlendPreset::Subtractive:
      alphaBlend            = true;
      colorBlendOperation   = BlendOperation::RevSub;
      sourceColorBlend      = BlendMode::SrcAlpha;
      destinationColorBlend = BlendMode::One;
      break;

    case BlendPreset::Min:
      alphaBlend            = true;
      colorBlendOperation   = BlendOperation::Min;
      sourceColorBlend      = BlendMode::SrcAlpha;
      destinationColorBlend = BlendMode::One;
      break;

    case BlendPreset::Max:
      alphaBlend            = true;
      colorBlendOperation   = BlendOperation::Max;
      sourceColorBlend      = BlendMode::SrcAlpha;
      destinationColorBlend = BlendMode::One;
      break;

    case BlendPreset::AlphaTest:
      alphaToCoverage = true;
      break;

    case BlendPreset::Blend:
      alphaBlend            = true;
      colorBlendOperation   = BlendOperation::Add;
      sourceColorBlend      = BlendMode::SrcAlpha;
      destinationColorBlend = BlendMode::InvSrcAlpha;
      break;

    case BlendPreset::AddRGB:
      alphaBlend            = true;
      colorBlendOperation   = BlendOperation::Add;
      sourceColorBlend      = BlendMode::One;
      destinationColorBlend = BlendMode::One;
      break;

    default:
      assert( false );
      break;
    }
  }

  bool           colorWrite            = true;
  bool           alphaToCoverage       = false;
  bool           alphaBlend            = false;
  BlendOperation colorBlendOperation   = BlendOperation::Add;
  BlendOperation alphaBlendOperation   = BlendOperation::Add;
  BlendMode      sourceColorBlend      = BlendMode::One;
  BlendMode      destinationColorBlend = BlendMode::One;
  BlendMode      sourceAlphaBlend      = BlendMode::One;
  BlendMode      destinationAlphaBlend = BlendMode::One;
};

struct DepthStencilDesc
{
  enum class Comparison
  {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always,
  };

  enum class StencilOperation
  {
    Keep,
    Zero,
    Replace,
    Invert,
    Increase,
    Decrease,
    IncSat,
    DecSat,
  };

  bool       depthTest     = true;
  bool       depthWrite    = true;
  Comparison depthFunction = Comparison::LessEqual;

  bool    stencilEnable    = false;
  uint8_t stencilReadMask  = 0xFF;
  uint8_t stencilWriteMask = 0xFF;

  StencilOperation stencilFrontPass      = StencilOperation::Keep;
  StencilOperation stencilFrontFail      = StencilOperation::Keep;
  StencilOperation stencilFrontDepthFail = StencilOperation::Keep;
  Comparison       stencilFrontFunction  = Comparison::Always;

  StencilOperation stencilBackPass      = StencilOperation::Keep;
  StencilOperation stencilBackFail      = StencilOperation::Keep;
  StencilOperation stencilBackDepthFail = StencilOperation::Keep;
  Comparison       stencilBackFunction  = Comparison::Always;
};

struct RasterizerDesc
{
  enum class CullMode
  {
    None,
    Front,
    Back,
  };

  enum class CullFront
  {
    Clockwise,
    CounterClockwise,
  };

  CullMode  cullMode  = CullMode::Back;
  CullFront cullFront = CullFront::CounterClockwise;
  int       depthBias = 0;
};

struct VertexDesc
{
  struct Element
  {
    enum class DataType
    {
      None,
      R16G16B16A16F,
      R16G16F,
      R10G10B10A1UN,
      R8G8B8A8U,
      R8G8B8A8UN,
      R32G32B32A32F,
      R32G32B32F,
      R32G32F,
      R32F,
      R32U,
    };

    DataType    dataType = DataType::None;
    int         dataOffset;
    char        elementName[ 20 ];
    int         elementIndex;
  };

  static constexpr int MaxElements = 8;

  Element elements[ MaxElements ];
  int     stride = 0;
};

struct PipelineDesc
{
  BlendDesc        blendDesc;
  DepthStencilDesc depthStencilDesc;
  RasterizerDesc   rasterizerDesc;
  VertexDesc       vertexDesc;
  const void*      vsData            = nullptr;
  const void*      psData            = nullptr;
  size_t           vsSize            = 0;
  size_t           psSize            = 0;
  PrimitiveType    primitiveType     = PrimitiveType::TriangleList;
  PixelFormat      targetFormat[ 8 ] = { PixelFormat::Unknown, PixelFormat::Unknown, PixelFormat::Unknown, PixelFormat::Unknown, PixelFormat::Unknown, PixelFormat::Unknown, PixelFormat::Unknown, PixelFormat::Unknown };
  PixelFormat      depthFormat       = PixelFormat::Unknown;
  int              samples           = 1;
  int              indexSize         = 16;
};

enum class PipelinePresets
{
  Depth,
  DepthOneBitAlpha,
  DepthWithHistory,
  DepthOneBitAlphaWithHistory,
  DepthWithMotionVectors,
  DepthOneBitAlphaWithMotionVectors,
  DepthWithHistoryWithMotionVectors,
  DepthOneBitAlphaWithHistoryWithMotionVectors,
  Mesh,
  MeshOneBitAlpha,
  MeshWithHistory,
  MeshOneBitAlphaWithHistory,
  MeshTranslucent,
  MeshTranslucentTwoSided,
  Skybox,
  Sprite,
  GIProbes,
  Gizmos,

  PresentCount
};

enum class ResourceType
{
  VertexBuffer,
  IndexBuffer,
  ConstantBuffer,
  Texture1D,
  Texture2D,
  Texture3D,
};

enum class HeapType
{
  Default,
  Upload,
  Readback,
};

struct RTInstance
{
  struct RTBottomLevelAccelerator* accel;
  XMFLOAT4X4*                      transform;
  int                              rtIBSlot;
  int                              rtVBSlot;
};

struct IndexRange
{
  int startIndex;
  int indexCount;
  int materialIx;
};

enum class AlphaModeCB : int;

using MaterialTranslator = std::function< void( int, int, int&, bool&, AlphaModeCB& ) >;
