#pragma once

#include "Types.h"
#include "Common/Compression.h"

#pragma pack( push, 1 )
#include "ShaderStructures.h"
#pragma pack( pop )

inline AlphaModeCB ConvertToAlphaMode( BlendPreset preset )
{
  switch ( preset )
  {
  case BlendPreset::DirectWrite: return AlphaModeCB::Opaque;
  case BlendPreset::AlphaTest:   return AlphaModeCB::OneBitAlpha;
  case BlendPreset::Blend:       return AlphaModeCB::Translucent;
  default:      assert( false ); return AlphaModeCB::Opaque;
  }
}