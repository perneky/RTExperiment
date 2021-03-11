#pragma once

inline std::vector< uint8_t > ReadFileToMemory( const wchar_t* filePath )
{
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