#pragma once

template< typename T, const wchar_t* name >
class PersistentValue
{
public:
  PersistentValue()
  {
    LoadSetting( name, value );
  }

  explicit PersistentValue( const T& initial )
  {
    value = initial;
    LoadSetting( name, value );
  }

  PersistentValue( const PersistentValue& ) = delete;
  PersistentValue( PersistentValue&& ) = delete;

  operator T& () { return value; }
  operator const T& () const { return value; }

  T* operator -> () { return &value; }
  const T* operator -> () const { return &value; }

  PersistentValue& operator = ( const T& newValue )
  { 
    value = newValue;
    SaveSetting( name, value );
    return *this; 
  }

  PersistentValue& operator = ( const PersistentValue& ) = delete;
  PersistentValue& operator = ( PersistentValue&& ) = delete;

private:
  T value;
};
