#pragma once

template< typename... ArgTypes >
class Signal
{
public:
  using FuncType = std::function< void( ArgTypes... ) >;
  using Token    = unsigned;

  ~Signal()
  {
    DisconnectAll();
  }

  Token Connect( FuncType func )
  {
    functions[ counter ] = func;
    return counter++;
  }

  void Disconnect( Token id )
  {
    auto iter = functions.find( id );
    assert( iter != functions.end() );
    functions.erase( iter );
  }

  template< unsigned len >
  void Disconnect( const Token( &ids )[ len ] )
  {
    for ( auto id : ids )
      Disconnect( id );
  }

  void DisconnectAll()
  {
    functions.clear();
  }

  void operator ()( ArgTypes... args )
  {
    SafeProcessContainer( functions, [ & ]( auto functor ) { functor->second( args... ); } );
  }

  struct AutoHandle
  {
    AutoHandle( Signal& sig, Token token ) : Sig( sig ), Token( token ) {}
    AutoHandle( AutoHandle&& other ) : Sig( other.Sig ), Token( other.Token ) { other.Sig = nullptr; other.Token = 0; }
    ~AutoHandle() { if ( Sig ) Sig->Disconnect( Token ); Sig = nullptr; }
    AutoHandle& operator = ( AutoHandle&& other ) { Sig = other.Sig; Token = other.Token; other.Sig = nullptr; other.Token = 0; return *this; }

    AutoHandle( const AutoHandle& ) = delete;
    AutoHandle& operator = ( const AutoHandle& ) = delete;

    Signal* Sig   = nullptr;
    Token   Token = 0;
  };

private:
  std::map< Token, FuncType > functions;
  Token                       counter = 1;
};
