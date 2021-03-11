#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include "assimp/inc/assimp/Importer.hpp"
#include "assimp/inc/assimp/scene.h"
#include "assimp/inc/assimp/postprocess.h"

#ifdef _DEBUG
# pragma comment( lib, "assimp/lib/assimp-vc142-mtd.lib" )
#else
# pragma comment( lib, "assimp/lib/assimp-vc142-mt.lib" )
#endif

#include "../External/tinyxml2/tinyxml2.h"

#include <vector>
#include <set>
#include <string>
#include <map>
#include <memory>
#include <functional>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <Shlwapi.h>
#pragma comment( lib, "Shlwapi.lib" )

using namespace tinyxml2;

template< typename T >
inline T clamp( T l, T g, T v )
{
  return std::min( g, std::max( l, v ) );
}

template< typename T >
inline T sqr( T v )
{
  return v * v;
}

inline std::string to_string( const GUID& guid )
{
  char guidStr[ 44 ];
  sprintf_s( guidStr, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}"
             , guid.Data1, guid.Data2, guid.Data3, guid.Data4[ 0 ], (int)guid.Data4[ 1 ]
             , (int)guid.Data4[ 2 ], (int)guid.Data4[ 3 ], (int)guid.Data4[ 4 ]
             , (int)guid.Data4[ 5 ], (int)guid.Data4[ 6 ], (int)guid.Data4[ 7 ] );

  return std::string( guidStr );

}

namespace Float16
{

using Type = uint16_t;

inline void Convert( uint16_t* __restrict out, const float in ) 
{
  uint32_t inu = *( (uint32_t*)&in );
  uint32_t t1;
  uint32_t t2;
  uint32_t t3;

  t1 = inu & 0x7fffffff;                 // Non-sign bits
  t2 = inu & 0x80000000;                 // Sign bit
  t3 = inu & 0x7f800000;                 // Exponent

  t1 >>= 13;                             // Align mantissa on MSB
  t2 >>= 16;                             // Shift sign bit into position

  t1 -= 0x1c000;                         // Adjust bias

  t1 = ( t3 < 0x38800000 ) ? 0 : t1;       // Flush-to-zero
  t1 = ( t3 > 0x47000000 ) ? 0x7bff : t1;  // Clamp-to-max
  t1 = ( t3 == 0 ? 0 : t1 );               // Denormals-as-zero

  t1 |= t2;                              // Re-insert sign bit

  *( (uint16_t*)out ) = t1;
}

}

