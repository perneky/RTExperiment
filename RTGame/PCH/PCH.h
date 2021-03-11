#pragma once

#include <memory>
#include <thread>
#include <cassert>
#include <mutex>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <filesystem>
#include <fstream>
#include <functional>
#include <array>
#include <optional>
#include <bitset>

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windowsx.h>
#include <shobjidl_core.h>

#include <atlbase.h>
#include <combaseapi.h>

#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>

#include <immintrin.h>

#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3d12.lib" )

#ifdef _DEBUG
# define DEBUG_GFX_API 1
#else
# define DEBUG_GFX_API 0
#endif // _DEBUG

#ifdef _DEBUG
# define ENABLE_GPU_DEBUG 1
#else
# define ENABLE_GPU_DEBUG 1
#endif // _DEBUG

#if ENABLE_GPU_DEBUG
# define USE_PIX
#endif // ENABLE_GPU_DEBUG

using namespace DirectX;
using namespace DirectX::PackedVector;

using DataChunk = std::vector< uint8_t >;

using IDXGIFactoryX = IDXGIFactory6;
using ID3D12DeviceX = ID3D12Device8;

template< typename T >
inline void ZeroObject( T& o )
{
  memset( &o, 0, sizeof( o ) );
}

template< typename T >
inline T Read( const uint8_t*& d )
{
  T t = *(T*)d;
  d += sizeof( t );
  return t;
}

template<>
inline std::wstring Read( const uint8_t*& d )
{
  std::wstring result;
  while ( char c = Read< char >( d ) )
    result += c;
  return result;
}

template<>
inline std::string Read( const uint8_t*& d )
{
  std::string result;
  while ( char c = Read< char >( d ) )
    result += c;
  return result;
}

template< typename T >
inline const T& Clamp( const T& min, const T& max, const T& value )
{
  return value <= min ? min : ( value >= max ? max : value );
}

inline double GetCPUTime()
{
  LARGE_INTEGER qpf, qpc;
  QueryPerformanceFrequency( &qpf );
  QueryPerformanceCounter( &qpc );
  return double( qpc.QuadPart ) / qpf.QuadPart;
}

template< typename T >
inline void SafeProcessContainer( T& container, std::function< void( typename T::iterator ) > proc )
{
  if ( container.empty() )
    return;

  std::vector< typename T::iterator > iters;
  iters.reserve( container.size() );
  for ( auto iter = container.begin(); iter != container.end(); ++iter )
    iters.emplace_back( iter );
  for ( auto& iter : iters )
    proc( iter );
}

inline float Random()
{
  return float( rand() ) / RAND_MAX;
}

inline bool ToVector3( const char* s, XMFLOAT3& v )
{
  if ( !s )
    return false;
  return sscanf_s( s, "%f;%f;%f", &v.x, &v.y, &v.z ) == 3;
}

inline bool ToColor( const char* s, XMFLOAT3& c )
{
  if ( !s )
    return false;

  if ( _stricmp( s, "White" ) == 0 )
    c = XMFLOAT3( 1, 1, 1 );
  else if ( _stricmp( s, "Black" ) == 0 )
    c = XMFLOAT3( 0, 0, 0 );
  else if ( _stricmp( s, "Red" ) == 0 )
    c = XMFLOAT3( 1, 0, 0 );
  else if ( _stricmp( s, "Green" ) == 0 )
    c = XMFLOAT3( 0, 1, 0 );
  else if ( _stricmp( s, "Blue" ) == 0 )
    c = XMFLOAT3( 0, 0, 1 );
  else if ( _stricmp( s, "Yellow" ) == 0 )
    c = XMFLOAT3( 1, 1, 0 );
  else if ( _stricmp( s, "Pink" ) == 0 )
    c = XMFLOAT3( 1, 0, 1 );
  else if ( _stricmp( s, "Cyan" ) == 0 )
    c = XMFLOAT3( 0, 1, 1 );
  else
    return sscanf_s( s, "%f;%f;%f", &c.x, &c.y, &c.z ) == 3;

  return true;
}

template< typename charType >
inline std::vector< std::basic_string< charType > > Split( const std::basic_string< charType >& text, charType separator )
{
  std::vector< std::basic_string< charType > > tokens;

  if ( text.empty() )
    return tokens;

  for ( size_t readCursor = 0; readCursor < text.size(); )
  {
    auto sepIx = text.find( separator, readCursor );
    if ( sepIx == std::wstring::npos )
    {
      tokens.push_back( text.substr( readCursor ) );
      return tokens;
    }
    else if ( sepIx == readCursor )
    {
      readCursor++;
    }
    else
    {
      tokens.push_back( text.substr( readCursor, sepIx - readCursor ) );
      readCursor = sepIx + 1;
    }
  }

  return tokens;
}

inline std::vector< std::string > Split( const char* text, char separator )
{
  return Split( std::string( text ), separator );
}

inline std::vector< std::wstring > Split( const wchar_t* text, wchar_t separator )
{
  return Split( std::wstring( text ), separator );
}

inline bool operator < ( const GUID& g1, const GUID& g2 )
{
  return memcmp( &g1, &g2, sizeof( g2 ) ) < 0;
}

inline bool operator > ( const GUID& g1, const GUID& g2 )
{
  return memcmp( &g1, &g2, sizeof( g2 ) ) > 0;
}

inline bool operator ! ( const GUID& guid )
{
  return guid.Data1 == 0 && guid.Data2 == 0 && guid.Data3 == 0 && *(uint64_t*)guid.Data4 == 0;
}

inline bool ParseGUID( const char* guidStr, GUID& guid )
{
  ZeroMemory( &guid, sizeof( guid ) );
  if ( !guidStr )
    return false;

  unsigned hx[ 8 ];

  auto result = sscanf_s( guidStr, "{%08lX-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}"
                        , &guid.Data1, &guid.Data2, &guid.Data3, &hx[ 0 ], &hx[ 1 ], &hx[ 2 ]
                        , &hx[ 3 ], &hx[ 4 ], &hx[ 5 ], &hx[ 6 ], &hx[ 7 ] );

  if ( result != 11 )
    result = sscanf_s( guidStr, "_%08lX_%04hX_%04hX_%02X%02X_%02X%02X%02X%02X%02X%02X_"
                     , &guid.Data1, &guid.Data2, &guid.Data3, &hx[ 0 ], &hx[ 1 ], &hx[ 2 ]
                     , &hx[ 3 ], &hx[ 4 ], &hx[ 5 ], &hx[ 6 ], &hx[ 7 ] );

  for ( int hi = 0; hi < 8; hi++ )
    guid.Data4[ hi ] = hx[ hi ];

  if ( result == 11 )
    return true;

  return false;
}

inline GUID ParseGUID( const char* guidStr )
{
  GUID guid;
  ParseGUID( guidStr, guid );
  return guid;
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

template< typename T, typename U >
inline int RoundUp( T numToRound, U multiple )
{
  if ( multiple == 0 )
    return numToRound;

  int remainder = numToRound % multiple;
  if ( remainder == 0 )
    return numToRound;

  return numToRound + multiple - remainder;
}

inline std::wstring W( const char* s )
{
  if ( !s || *s == 0 )
    return std::wstring();

  int len = int( strlen( s ) );

  auto wideCharCount = MultiByteToWideChar( CP_UTF8, 0, s, len, nullptr, 0 );
  std::vector< wchar_t > wideChars( wideCharCount );
  MultiByteToWideChar( CP_UTF8, 0, s, len, wideChars.data(), int( wideChars.size() ) );
  return std::wstring( wideChars.data(), wideChars.size() );
}

inline std::string N( const wchar_t* s )
{
  if ( !s || *s == 0 )
    return std::string();

  int len = int( wcslen( s ) );

  auto narrowCharCount = WideCharToMultiByte( CP_UTF8, 0, s, len, nullptr, 0, nullptr, nullptr );
  std::vector< char > narrowChars( narrowCharCount );
  WideCharToMultiByte( CP_UTF8, 0, s, len, narrowChars.data(), int( narrowChars.size() ), nullptr, nullptr );
  return std::string( narrowChars.data(), narrowChars.size() );
}

static constexpr GUID d3dContainerGUID = { 0x54837841, 0x5234, 0x9721, { 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34 } };

template< typename T >
inline void SetContainerObject( ID3D12Object* object, T* container )
{
  object->SetPrivateData( d3dContainerGUID, sizeof( T* ), (void*)&container );
}

template< typename T >
inline T* GetContainerObject( ID3D12Object* object )
{
  T*   container = nullptr;
  UINT dataSize  = sizeof( T* );
  object->GetPrivateData( d3dContainerGUID, &dataSize, (void*)&container );
  assert( container );
  assert( dataSize == sizeof( T* ) );
  return container;
}

inline bool SaveSetting( const wchar_t* name, unsigned long value )
{
  return RegSetKeyValueW( HKEY_CURRENT_USER, L"Software\\Brumi\\RTGame", name, REG_DWORD, &value, sizeof( value ) ) == ERROR_SUCCESS;
}

inline bool SaveSetting( const wchar_t* name, bool value )
{
  return SaveSetting( name, value ? 1LU : 0LU );
}

inline bool SaveSetting( const wchar_t* name, const wchar_t* value )
{
  return RegSetKeyValueW( HKEY_CURRENT_USER, L"Software\\Brumi\\RTGame", name, REG_SZ, value, DWORD( wcslen( value ) + 1 ) * sizeof( wchar_t ) ) == ERROR_SUCCESS;
}

inline bool SaveSetting( const wchar_t* name, const std::wstring& value )
{
  return SaveSetting( name, value.data() );
}

inline bool LoadSetting( const wchar_t* name, unsigned long& value )
{
  HKEY key;
  if ( RegOpenKeyExW( HKEY_CURRENT_USER, L"Software\\Brumi\\RTGame", 0, KEY_READ, &key ) != ERROR_SUCCESS )
    return false;

  DWORD dataSize = sizeof( value );
  auto hr = RegQueryValueExW( key, name, 0, nullptr, (LPBYTE)&value, &dataSize );
  RegCloseKey( key );
  
  return hr == ERROR_SUCCESS;
}

inline bool LoadSetting( const wchar_t* name, bool& value )
{
  unsigned long l;
  if ( LoadSetting( name, l ) )
  {
    value = l != 0;
    return true;
  }

  return false;
}

inline bool LoadSetting( const wchar_t* name, std::wstring& value )
{
  HKEY key;
  if ( RegOpenKeyExW( HKEY_CURRENT_USER, L"Software\\Brumi\\RTGame", 0, KEY_READ, &key ) != ERROR_SUCCESS )
    return false;

  DWORD dataSize = 0;
  auto hr = RegQueryValueExW( key, name, 0, nullptr, nullptr, &dataSize );
  if ( hr == ERROR_SUCCESS )
  {
    value.resize( dataSize / sizeof( wchar_t ) + 1 );
    hr = RegQueryValueExW( key, name, 0, nullptr, (LPBYTE)value.data(), &dataSize );
  }

  RegCloseKey( key );

  return hr == ERROR_SUCCESS;
}

inline XMVECTOR ClosestPointTo( FXMVECTOR point, const BoundingOrientedBox& obb )
{
  auto center = XMLoadFloat3( &obb.Center );
  auto orient = XMMatrixTranspose( XMMatrixRotationQuaternion( XMLoadFloat4( &obb.Orientation ) ) );
  auto xAxis  = orient.r[ 0 ];
  auto yAxis  = orient.r[ 1 ];
  auto zAxis  = orient.r[ 2 ];

  auto directionVector = point - center;

  auto distanceX = XMVectorGetX( XMVector3Dot( directionVector, xAxis ) );
  if ( distanceX > obb.Extents.x ) distanceX = obb.Extents.x;
  else if ( distanceX < -obb.Extents.x ) distanceX = -obb.Extents.x;

  auto distanceY = XMVectorGetX( XMVector3Dot( directionVector, yAxis ) );
  if ( distanceY > obb.Extents.y ) distanceY = obb.Extents.y;
  else if ( distanceY < -obb.Extents.y ) distanceY = -obb.Extents.y;

  auto distanceZ = XMVectorGetX( XMVector3Dot( directionVector, zAxis ) );
  if ( distanceZ > obb.Extents.z ) distanceZ = obb.Extents.z;
  else if ( distanceZ < -obb.Extents.z ) distanceZ = -obb.Extents.z;

  return center + distanceX * xAxis + distanceY * yAxis + distanceZ * zAxis;
}