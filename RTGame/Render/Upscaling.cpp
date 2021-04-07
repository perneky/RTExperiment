#include "Upscaling.h"
#include "ComputeShader.h"
#include "DLSSUpscaling.h"

std::unique_ptr< Upscaling > Upscaling::Instantiate()
{
  if ( DLSSUpscaling::IsAvailable() )
    return std::make_unique< DLSSUpscaling >();

  return nullptr;
}
