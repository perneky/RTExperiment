#include "Upscaling.h"
#include "ComputeShader.h"
#include "LinearUpscaling.h"

std::unique_ptr< Upscaling > Upscaling::Instantiate()
{
  return std::make_unique< LinearUpscaling >();
}
