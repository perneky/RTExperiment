#pragma once

namespace tinyxml2 { class XMLElement; }

class AnimationSet
{
public:
  AnimationSet( std::vector< uint8_t > data );

  void AddAnimation( const wchar_t* name, const wchar_t* group, int start, int end );

  int            GetAnimationCount() const;
  const wchar_t* GetAnimationName( size_t index ) const;
  int            GetAnimationIndex( const wchar_t* name ) const;
  int            GetAnimationLength( size_t index ) const;

  void Evaluate( int index, float rate, float tps, std::map< std::wstring, XMFLOAT4X4 >& nodes ) const;

private:
  struct VectorKey
  {
    int tick;
    XMFLOAT3 value;

    VectorKey() = default;
    VectorKey( int tick, const XMFLOAT3& value ) : tick( tick ), value( value ) {}

    static bool less( int t, const VectorKey& k ) { return t < k.tick; }
  };
  struct QuatKey
  {
    uint32_t tick;
    XMFLOAT4 value;

    QuatKey() = default;
    QuatKey( uint32_t tick, const XMFLOAT4& value ) : tick( tick ), value( value ) {}

    static bool less( uint32_t t, const QuatKey& k ) { return t < k.tick; }
  };

  using VectorKeys = std::vector< VectorKey >;
  using QuatKeys   = std::vector< QuatKey   >;

  struct Influence
  {
    std::wstring bone;

    VectorKeys sclKeys;
    QuatKeys   rotKeys;
    VectorKeys posKeys;
  };

  using Influences     = std::map< std::wstring, Influence >;
  using InfluenceGroup = std::map< std::wstring, Influences >;

  struct Animation
  {
    std::wstring name;
    std::wstring group;

    uint32_t startTick;
    uint32_t endTick;
  };

  using Animations = std::vector< Animation >;

  struct Frame
  {
    std::wstring name;
    XMFLOAT4X4   baseXForm;

    std::vector< Frame > children;
  };

  using Frames = std::vector< Frame >;

  InfluenceGroup influences;
  Animations     animations;
  Frames         frames;

  void ParseFrame( const tinyxml2::XMLElement& xmlFrame, AnimationSet::Frame* parentFrame );
  void EvaluateNode( FXMMATRIX parentXform, const Frame& frame, const Influences& infs, int tick, std::map< std::wstring, XMFLOAT4X4 >& nodes ) const;
};
