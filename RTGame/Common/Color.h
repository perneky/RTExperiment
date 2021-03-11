#pragma once

struct Color
{
  Color() = default;
  Color( float r, float g, float b, float a = 1 ) : r( r ), g( g ), b( b ), a( a ) {}

  float r = 0;
  float g = 0;
  float b = 0;
  float a = 1;
};