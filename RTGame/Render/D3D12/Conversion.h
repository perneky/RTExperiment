#pragma once

#include "../Types.h"

inline D3D12_COMMAND_LIST_TYPE Convert( CommandQueueType type )
{
  switch ( type )
  {
  case CommandQueueType::Direct:  return D3D12_COMMAND_LIST_TYPE_DIRECT;
  case CommandQueueType::Compute: return D3D12_COMMAND_LIST_TYPE_COMPUTE;
  case CommandQueueType::Copy:    return D3D12_COMMAND_LIST_TYPE_COPY;
  default:       assert( false ); return D3D12_COMMAND_LIST_TYPE_DIRECT;
  }
}

inline D3D12_RESOURCE_STATES Convert( ResourceState state )
{
  D3D12_RESOURCE_STATES d3dStates = D3D12_RESOURCE_STATE_COMMON;

#define HandleBit( wb, nb ) { if ( ( state.bits & ResourceStateBits::wb ) != 0 ) d3dStates |= nb; }

  HandleBit( VertexOrConstantBuffer , D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER );
  HandleBit( IndexBuffer            , D3D12_RESOURCE_STATE_INDEX_BUFFER );
  HandleBit( RenderTarget           , D3D12_RESOURCE_STATE_RENDER_TARGET );
  HandleBit( UnorderedAccess        , D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
  HandleBit( DepthWrite             , D3D12_RESOURCE_STATE_DEPTH_WRITE  );
  HandleBit( DepthRead              , D3D12_RESOURCE_STATE_DEPTH_READ  );
  HandleBit( PixelShaderInput       , D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE );
  HandleBit( NonPixelShaderInput    , D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE );
  HandleBit( StreamOut              , D3D12_RESOURCE_STATE_STREAM_OUT );
  HandleBit( IndirectArgument       , D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT );
  HandleBit( CopySource             , D3D12_RESOURCE_STATE_COPY_SOURCE );
  HandleBit( CopyDestination        , D3D12_RESOURCE_STATE_COPY_DEST );
  HandleBit( ResolveSource          , D3D12_RESOURCE_STATE_RESOLVE_SOURCE );
  HandleBit( ResolveDestination     , D3D12_RESOURCE_STATE_RESOLVE_DEST );
  HandleBit( RTAccelerationStructure, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE );
  HandleBit( ShadingRateSource      , D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE );
  HandleBit( GenericRead            , D3D12_RESOURCE_STATE_GENERIC_READ );
  HandleBit( Present                , D3D12_RESOURCE_STATE_PRESENT );
  HandleBit( Predication            , D3D12_RESOURCE_STATE_PREDICATION );

#undef HandleBit

  return d3dStates;
}

inline DXGI_FORMAT Convert( PixelFormat format )
{
  switch ( format )
  {
  case PixelFormat::R8UN:          return DXGI_FORMAT_R8_UNORM;
  case PixelFormat::A8UN:          return DXGI_FORMAT_A8_UNORM;
  case PixelFormat::RG88UN:        return DXGI_FORMAT_R8G8_UNORM;
  case PixelFormat::RGBA8888UN:    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case PixelFormat::BGRA8888UN:    return DXGI_FORMAT_B8G8R8A8_UNORM;
  case PixelFormat::RGBA1010102UN: return DXGI_FORMAT_R10G10B10A2_UNORM;
  case PixelFormat::RGBA16161616F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
  case PixelFormat::RG1616F:       return DXGI_FORMAT_R16G16_FLOAT;
  case PixelFormat::R16UN:         return DXGI_FORMAT_R16_UNORM;
  case PixelFormat::BC1UN:         return DXGI_FORMAT_BC1_UNORM;
  case PixelFormat::BC2UN:         return DXGI_FORMAT_BC2_UNORM;
  case PixelFormat::BC3UN:         return DXGI_FORMAT_BC3_UNORM;
  case PixelFormat::BC4UN:         return DXGI_FORMAT_BC4_UNORM;
  case PixelFormat::BC5UN:         return DXGI_FORMAT_BC5_UNORM;
  case PixelFormat::RGBA1010102U:  return DXGI_FORMAT_R10G10B10A2_UINT;
  case PixelFormat::D16:           return DXGI_FORMAT_R16_TYPELESS;
  case PixelFormat::D24S8:         return DXGI_FORMAT_R24G8_TYPELESS;
  case PixelFormat::D32:           return DXGI_FORMAT_R32_TYPELESS;
  default:        assert( false ); return DXGI_FORMAT_UNKNOWN;
  }
}

inline DXGI_FORMAT ConvertForDSV( DXGI_FORMAT format )
{
  switch ( format )
  {
  case DXGI_FORMAT_R16_TYPELESS:   return DXGI_FORMAT_D16_UNORM;
  case DXGI_FORMAT_R24G8_TYPELESS: return DXGI_FORMAT_D24_UNORM_S8_UINT;
  case DXGI_FORMAT_R32_TYPELESS:   return DXGI_FORMAT_D32_FLOAT;
  default:                         return format;
  }

}

inline DXGI_FORMAT ConvertForSRV( DXGI_FORMAT format )
{
  switch ( format )
  {
  case DXGI_FORMAT_R16_TYPELESS:   return DXGI_FORMAT_R16_FLOAT;
  case DXGI_FORMAT_R24G8_TYPELESS: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
  case DXGI_FORMAT_R32_TYPELESS:   return DXGI_FORMAT_R32_FLOAT;
  default:                         return format;
  }

}

inline int CalcTexelSize( PixelFormat format )
{
  switch ( format )
  {
  case PixelFormat::R8UN:
  case PixelFormat::A8UN:
    return 1;
  case PixelFormat::RG88UN:
  case PixelFormat::R16UN:
    return 2;
  case PixelFormat::RGBA8888UN:
  case PixelFormat::RGBA1010102UN:
  case PixelFormat::RGBA1010102U:
  case PixelFormat::RG1616F:
    return 4;
  case PixelFormat::RGBA16161616F:
    return 8;
  default:
    assert( false );
    return 1;
  }
}

inline D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertForRootSignature( PrimitiveType type )
{
  switch ( type )
  {
  case PrimitiveType::PointList:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  case PrimitiveType::LineList:      return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  case PrimitiveType::LineStrip:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  case PrimitiveType::TriangleList:  return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  case PrimitiveType::TriangleStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  default:          assert( false ); return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  }
}

inline DXGI_FORMAT Convert( VertexDesc::Element::DataType type )
{
  switch ( type )
  {
  case VertexDesc::Element::DataType::R16G16B16A16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
  case VertexDesc::Element::DataType::R16G16F:       return DXGI_FORMAT_R16G16_FLOAT;
  case VertexDesc::Element::DataType::R10G10B10A1UN: return DXGI_FORMAT_R10G10B10A2_UNORM;
  case VertexDesc::Element::DataType::R8G8B8A8U:     return DXGI_FORMAT_R8G8B8A8_UINT;
  case VertexDesc::Element::DataType::R8G8B8A8UN:    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case VertexDesc::Element::DataType::R32G32B32A32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
  case VertexDesc::Element::DataType::R32G32B32F:    return DXGI_FORMAT_R32G32B32_FLOAT;
  case VertexDesc::Element::DataType::R32G32F:       return DXGI_FORMAT_R32G32_FLOAT;
  case VertexDesc::Element::DataType::R32F:          return DXGI_FORMAT_R32_FLOAT;
  case VertexDesc::Element::DataType::R32U:          return DXGI_FORMAT_R32_UINT;
  default:                          assert( false ); return DXGI_FORMAT_UNKNOWN;
  }
}

inline D3D12_BLEND_OP Convert( BlendDesc::BlendOperation op )
{
  switch ( op )
  {
  case BlendDesc::BlendOperation::Add:    return D3D12_BLEND_OP_ADD;
  case BlendDesc::BlendOperation::Sub:    return D3D12_BLEND_OP_SUBTRACT;
  case BlendDesc::BlendOperation::RevSub: return D3D12_BLEND_OP_REV_SUBTRACT;
  case BlendDesc::BlendOperation::Min:    return D3D12_BLEND_OP_MIN;
  case BlendDesc::BlendOperation::Max:    return D3D12_BLEND_OP_MAX;
  default:                                return D3D12_BLEND_OP_ADD;
  }
}

inline D3D12_BLEND Convert( BlendDesc::BlendMode mode )
{
  switch ( mode )
  {
  case BlendDesc::BlendMode::Zero:         return D3D12_BLEND_ZERO;
  case BlendDesc::BlendMode::One:          return D3D12_BLEND_ONE;
  case BlendDesc::BlendMode::SrcColor:     return D3D12_BLEND_SRC_COLOR;
  case BlendDesc::BlendMode::InvSrcColor:  return D3D12_BLEND_INV_SRC_COLOR;
  case BlendDesc::BlendMode::SrcAlpha:     return D3D12_BLEND_SRC_ALPHA;
  case BlendDesc::BlendMode::InvSrcAlpha:  return D3D12_BLEND_INV_SRC_ALPHA;
  case BlendDesc::BlendMode::DestColor:    return D3D12_BLEND_DEST_COLOR;
  case BlendDesc::BlendMode::InvDestColor: return D3D12_BLEND_INV_DEST_COLOR;
  case BlendDesc::BlendMode::DestAlpha:    return D3D12_BLEND_DEST_ALPHA;
  case BlendDesc::BlendMode::InvDestAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
  default:                                 return D3D12_BLEND_ZERO;
  }
}

inline D3D12_COMPARISON_FUNC Convert( DepthStencilDesc::Comparison cmp )
{
  switch ( cmp )
  {
  case DepthStencilDesc::Comparison::Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
  case DepthStencilDesc::Comparison::Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
  case DepthStencilDesc::Comparison::Greater:      return D3D12_COMPARISON_FUNC_GREATER;
  case DepthStencilDesc::Comparison::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
  case DepthStencilDesc::Comparison::Less:         return D3D12_COMPARISON_FUNC_LESS;
  case DepthStencilDesc::Comparison::LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
  case DepthStencilDesc::Comparison::Never:        return D3D12_COMPARISON_FUNC_NEVER;
  case DepthStencilDesc::Comparison::NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
  default:                                         return D3D12_COMPARISON_FUNC_ALWAYS;
  }
}

inline D3D12_STENCIL_OP Convert( DepthStencilDesc::StencilOperation operation )
{
  switch ( operation )
  {
  case DepthStencilDesc::StencilOperation::Decrease: return D3D12_STENCIL_OP_DECR;
  case DepthStencilDesc::StencilOperation::DecSat:   return D3D12_STENCIL_OP_DECR_SAT;
  case DepthStencilDesc::StencilOperation::Increase: return D3D12_STENCIL_OP_INCR;
  case DepthStencilDesc::StencilOperation::IncSat:   return D3D12_STENCIL_OP_INCR_SAT;
  case DepthStencilDesc::StencilOperation::Invert:   return D3D12_STENCIL_OP_INVERT;
  case DepthStencilDesc::StencilOperation::Keep:     return D3D12_STENCIL_OP_KEEP;
  case DepthStencilDesc::StencilOperation::Replace:  return D3D12_STENCIL_OP_REPLACE;
  case DepthStencilDesc::StencilOperation::Zero:     return D3D12_STENCIL_OP_ZERO;
  default:                                           return D3D12_STENCIL_OP_KEEP;
  }
}

inline D3D12_CULL_MODE Convert( RasterizerDesc::CullMode mode )
{
  switch ( mode )
  {
  case RasterizerDesc::CullMode::None:  return D3D12_CULL_MODE_NONE;
  case RasterizerDesc::CullMode::Front: return D3D12_CULL_MODE_FRONT;
  case RasterizerDesc::CullMode::Back:  return D3D12_CULL_MODE_BACK;
  default:                              return D3D12_CULL_MODE_BACK;
  }
}

inline D3D12_HEAP_TYPE Convert( HeapType heapType )
{
  switch ( heapType )
  {
  case HeapType::Default:   return D3D12_HEAP_TYPE_DEFAULT;
  case HeapType::Upload:    return D3D12_HEAP_TYPE_UPLOAD;
  default: assert( false ); return D3D12_HEAP_TYPE_DEFAULT;
  }
}

inline D3D12_PRIMITIVE_TOPOLOGY Convert( PrimitiveType type )
{
  switch ( type )
  {
  case PrimitiveType::PointList:     return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
  case PrimitiveType::LineList:      return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
  case PrimitiveType::LineStrip:     return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
  case PrimitiveType::TriangleList:  return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  case PrimitiveType::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
  default:          assert( false ); return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
  }
}
