#include "DLSSUpscaling.h"
#include "Device.h"
#include "CommandList.h"
#include "Resource.h"
#include "RenderManager.h"
#include "D3D12/D3DDevice.h"
#include "D3D12/D3DCommandList.h"
#include "D3D12/D3DResource.h"
#include "Halton.h"

#include "DLSS/Include/nvsdk_ngx.h"
#include "DLSS/Include/nvsdk_ngx_helpers.h"

uint64_t nvAppId = 2875904;

static bool CheckNVAPI( NVSDK_NGX_Result result )
{
  if ( result == NVSDK_NGX_Result_Success )
    return true;

  int errorCode = result & ~(int)NVSDK_NGX_Result_Fail;

  assert( false );
  return false;
}

static NVSDK_NGX_PerfQuality_Value Convert( Upscaling::Quality quality )
{
  switch ( quality )
  {
  case Upscaling::Quality::UltraQuality:
    return NVSDK_NGX_PerfQuality_Value_UltraQuality;

  case Upscaling::Quality::Quality:
    return NVSDK_NGX_PerfQuality_Value_MaxQuality;

  case Upscaling::Quality::Performance:
    return NVSDK_NGX_PerfQuality_Value_MaxPerf;

  case Upscaling::Quality::UltraPerformance:
    return NVSDK_NGX_PerfQuality_Value_UltraPerformance;

  case Upscaling::Quality::Balanced:
  default:
    return NVSDK_NGX_PerfQuality_Value_Balanced;

  }
}

DLSSUpscaling::~DLSSUpscaling() = default;

void DLSSUpscaling::Initialize( CommandList& commandList, Quality quality, int outputWidth, int outputHeight )
{
  this->quality = quality;

  auto d3dDevice      = reinterpret_cast< D3DDevice*      >( &RenderManager::GetInstance().GetDevice() );
  auto d3dCommandList = reinterpret_cast< D3DCommandList* >( &commandList );

  NVSDK_NGX_FeatureCommonInfo info = { {0} };
  CheckNVAPI( NVSDK_NGX_D3D12_Init( nvAppId, L"./Logs", d3dDevice->GetD3DDevice(), &info ) );

  CheckNVAPI( NVSDK_NGX_D3D12_GetCapabilityParameters( &params ) );

  CheckNVAPI( NGX_DLSS_GET_OPTIMAL_SETTINGS( params
                                           , outputWidth
                                           , outputHeight
                                           , Convert( quality )
                                           , reinterpret_cast<unsigned int*>( &renderWidth )
                                           , reinterpret_cast<unsigned int*>( &renderHeight )
                                           , reinterpret_cast<unsigned int*>( &renderMaxWidth )
                                           , reinterpret_cast<unsigned int*>( &renderMaxHeight )
                                           , reinterpret_cast<unsigned int*>( &renderMinWidth )
                                           , reinterpret_cast<unsigned int*>( &renderMinHeight )
                                           , &sharpness ) );

  NVSDK_NGX_DLSS_Create_Params dlssCreateParams = {};
  dlssCreateParams.Feature.InWidth            = renderWidth;
  dlssCreateParams.Feature.InHeight           = renderHeight;
  dlssCreateParams.Feature.InTargetWidth      = outputWidth;
  dlssCreateParams.Feature.InTargetHeight     = outputHeight;
  dlssCreateParams.Feature.InPerfQualityValue = Convert( quality );
  dlssCreateParams.InEnableOutputSubrects     = false;
  dlssCreateParams.InFeatureCreateFlags       = NVSDK_NGX_DLSS_Feature_Flags_IsHDR 
                                              | NVSDK_NGX_DLSS_Feature_Flags_MVLowRes
                                              | NVSDK_NGX_DLSS_Feature_Flags_DoSharpening;

  CheckNVAPI( NGX_D3D12_CREATE_DLSS_EXT( d3dCommandList->GetD3DGraphicsCommandList(), 1, 1, &dlss, params, &dlssCreateParams ) );

  d3dDevice->SetTextureLODBias( log2( float( renderWidth ) / outputWidth ) + 0.0f );

  jitterPhase = 0;

  int basePhaseCount = 8;
  int baseValues[]   = { 2, 3 };
  jitterSequence.resize( size_t( basePhaseCount * pow( float( outputHeight ) / renderHeight, 2 ) ) );

  for ( int hi = 0; hi < int( jitterSequence.size() ); ++hi )
  {
    auto vals = halton_base( hi + 1, 2, baseValues );
    jitterSequence[ hi ].x = vals[ 0 ] - 0.5f;
    jitterSequence[ hi ].y = vals[ 1 ] - 0.5f;
  }
}

void DLSSUpscaling::TearDown( CommandList& commandList )
{
  RenderManager::GetInstance().IdleGPU();

  if ( params )
    CheckNVAPI( NVSDK_NGX_D3D12_DestroyParameters( params ) );
  if ( dlss )
    CheckNVAPI( NVSDK_NGX_D3D12_ReleaseFeature( dlss ) );

  CheckNVAPI( NVSDK_NGX_D3D12_Shutdown() );
}

XMINT2 DLSSUpscaling::GetRenderingResolution() const
{
  return XMINT2( renderWidth, renderHeight );
}

XMFLOAT2 DLSSUpscaling::GetJitter() const
{
  return jitterSequence[ jitterPhase ];
}

void DLSSUpscaling::Upscale( CommandList& commandList
                           , Resource& lowResColor
                           , Resource& lowResDepth
                           , Resource& lowResMotionVectors
                           , Resource& highResColor
                           , Resource& exposure
                           , float prevExposure
                           , const XMFLOAT2& jitter )
{
  commandList.ChangeResourceState( lowResColor,         ResourceStateBits::PixelShaderInput );
  commandList.ChangeResourceState( lowResDepth,         ResourceStateBits::PixelShaderInput );
  commandList.ChangeResourceState( lowResMotionVectors, ResourceStateBits::PixelShaderInput );

  commandList.ChangeResourceState( highResColor, ResourceStateBits::UnorderedAccess );

  auto d3dCommandList         = reinterpret_cast< D3DCommandList* >( &commandList );
  auto d3dLowResColor         = reinterpret_cast< D3DResource*    >( &lowResColor );
  auto d3dLowResDepth         = reinterpret_cast< D3DResource*    >( &lowResDepth );
  auto d3dLowResMotionVectors = reinterpret_cast< D3DResource*    >( &lowResMotionVectors );
  auto d3dHighResColor        = reinterpret_cast< D3DResource*    >( &highResColor );
  auto d3dExposure            = reinterpret_cast< D3DResource*    >( &exposure );

  NVSDK_NGX_D3D12_DLSS_Eval_Params evalParams = {};
  evalParams.Feature.InSharpness              = sharpness;
  evalParams.Feature.pInColor                 = d3dLowResColor->GetD3DResource();
  evalParams.Feature.pInOutput                = d3dHighResColor->GetD3DResource();
  evalParams.pInDepth                         = d3dLowResDepth->GetD3DResource();
  evalParams.pInMotionVectors                 = d3dLowResMotionVectors->GetD3DResource();
  evalParams.InRenderSubrectDimensions.Width  = d3dLowResColor->GetTextureWidth();
  evalParams.InRenderSubrectDimensions.Height = d3dLowResColor->GetTextureHeight();
  evalParams.InMVScaleX                       = 1.0f;
  evalParams.InMVScaleY                       = 1.0f;
  evalParams.InJitterOffsetX                  = jitter.x;
  evalParams.InJitterOffsetY                  = jitter.y;
  evalParams.pInExposureTexture               = d3dExposure->GetD3DResource();
  evalParams.InPreExposure                    = prevExposure;

  CheckNVAPI( NGX_D3D12_EVALUATE_DLSS_EXT( d3dCommandList->GetD3DGraphicsCommandList(), dlss, params, &evalParams ) );

  commandList.BindHeaps( RenderManager::GetInstance().GetDevice() );

  jitterPhase = ++jitterPhase % jitterSequence.size();
}

bool DLSSUpscaling::IsAvailable()
{
  if ( auto d3dDevice = reinterpret_cast<D3DDevice*>( &RenderManager::GetInstance().GetDevice() ) )
  {
    NVSDK_NGX_FeatureCommonInfo info = { {0} };
    if ( !CheckNVAPI( NVSDK_NGX_D3D12_Init( nvAppId, L"./Logs", d3dDevice->GetD3DDevice(), &info ) ) )
      return false;

    CheckNVAPI( NVSDK_NGX_D3D12_Shutdown() );

    return true;
  }

  return false;
}
