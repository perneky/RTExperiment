#include "LinearUpscaling.h"
#include "ComputeShader.h"
#include "Device.h"
#include "CommandList.h"
#include "Resource.h"
#include "RenderManager.h"
#include "Common/Files.h"

LinearUpscaling::~LinearUpscaling() = default;

void LinearUpscaling::Initialize( Quality quality, CommandList& commandList )
{
  this->quality = quality;

  auto& device = RenderManager::GetInstance().GetDevice();

  auto copierFile = ReadFileToMemory( L"Content/Shaders/LinearUpscaling.cso" );
  copier = device.CreateComputeShader( copierFile.data(), int( copierFile.size() ), L"LinearUpsampling" );
}

void LinearUpscaling::TearDown( CommandList& commandList )
{
  copier.reset();
}

XMINT2 LinearUpscaling::CalcLowResolution( int highResolutionWidth, int highResolutionHeight ) const
{
  switch ( quality )
  {
  case Quality::Quality:
    return XMINT2( highResolutionWidth, highResolutionHeight );

  case Quality::Balanced:
    return XMINT2( ( 8 * highResolutionWidth ) / 10, ( 8 * highResolutionHeight ) / 10 );

  case Quality::Performance:
    return XMINT2( ( 6 * highResolutionWidth ) / 10, ( 6 * highResolutionHeight ) / 10 );

  case Quality::UltraPerformance:
  default:
    return XMINT2( ( 5 * highResolutionWidth ) / 10, ( 5 * highResolutionHeight ) / 10 );
  }
}

void LinearUpscaling::Upscale( CommandList& commandList, Resource& lowResColor, Resource& lowResDepth, Resource& lowResMotionVectors, Resource& highResColor )
{
  commandList.SetComputeShader( *copier );

  struct CopyArgs
  {
    XMFLOAT2 invDstTexSize;
  } args;

  args.invDstTexSize.x = 1.0f / highResColor.GetTextureWidth();
  args.invDstTexSize.y = 1.0f / highResColor.GetTextureHeight();

  commandList.ChangeResourceState( lowResColor, ResourceStateBits::NonPixelShaderInput );
  commandList.ChangeResourceState( highResColor, ResourceStateBits::UnorderedAccess );

  commandList.SetComputeConstantValues( 0, args );

  commandList.SetComputeResource( 1, *lowResColor.GetResourceDescriptor( ResourceDescriptorType::ShaderResourceView ) );
  commandList.SetComputeResource( 2, *highResColor.GetResourceDescriptor( ResourceDescriptorType::UnorderedAccessView ) );

  commandList.Dispatch( ( highResColor.GetTextureWidth()  + LinearUSKernelWidth  - 1 ) / LinearUSKernelWidth
                      , ( highResColor.GetTextureHeight() + LinearUSKernelHeight - 1 ) / LinearUSKernelHeight
                      , 1 );
}
