#pragma once

inline std::vector< uint8_t > ReadFileToMemory( const wchar_t* filePath )
{
#ifdef _DEBUG
  std::wstring debugPath( filePath );
  if ( debugPath.ends_with( L".cso" ) )
  {
    debugPath.insert( debugPath.size() - 4, L"_d" );
    filePath = debugPath.data();
  }
#endif

  std::ifstream   file( filePath, std::ios::binary | std::ios::ate );
  std::streamsize size = file.tellg();
  file.seekg( 0, std::ios::beg );

  if ( size < 0 )
    return {};

  std::vector< uint8_t > buffer( size );
  if ( file.read( (char*)buffer.data(), size ) )
    return buffer;

  return {};
}