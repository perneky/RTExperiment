#pragma once

struct Finally
{
  using Func = std::function< void() >;

  Finally() = default;
  Finally( Func&& func ) : func( func ) {}
  ~Finally() { if ( func ) func(); }

  Finally( const Finally& ) = delete;
  Finally( const Finally&& other ) : func( std::move( other.func ) ) {}
  Finally& operator = ( const Finally& ) = delete;
  Finally& operator = ( const Finally&& other ) { func = std::move( other.func ); }

  Func func;
};