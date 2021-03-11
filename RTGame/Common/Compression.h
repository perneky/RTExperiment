#pragma once

#pragma pack( push, 1 )
struct DirVect
{
  static uint32_t Compress( float f )
  {
    return unsigned( Clamp( 0.0f, 1.0f, f * 0.5f + 0.5f ) * 1023 );
  }

  DirVect()
  {
  }
  DirVect( float x, float y, float z )
    : x( Compress( x ) )
    , y( Compress( y ) )
    , z( Compress( z ) )
    , w( 3 )
  {
  }

  unsigned x : 10;
  unsigned y : 10;
  unsigned z : 10;
  unsigned w : 2;
};
#pragma pack( pop )
