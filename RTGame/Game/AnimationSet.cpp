#include "AnimationSet.h"
#include "Render/ModelFeatures.h"
#include "tinyxml2.h"

using namespace tinyxml2;

void AnimationSet::ParseFrame( const XMLElement& xmlFrame, AnimationSet::Frame* parentFrame )
{
  for ( auto xmlChild = xmlFrame.FirstChildElement( "Frame" ); xmlChild; xmlChild = xmlChild->NextSiblingElement( "Frame" ) )
  {
    Frame* newFrame = nullptr;
    if ( parentFrame )
    {
      parentFrame->children.emplace_back();
      newFrame = &parentFrame->children.back();
    }
    else
    {
      frames.emplace_back();
      newFrame = &frames.back();
    }

    newFrame->name = W( xmlChild->Attribute( "name" ) );
    sscanf_s( xmlChild->Attribute( "xform" ), "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f"
            , &newFrame->baseXForm._11
            , &newFrame->baseXForm._21
            , &newFrame->baseXForm._31
            , &newFrame->baseXForm._41
            , &newFrame->baseXForm._12
            , &newFrame->baseXForm._22
            , &newFrame->baseXForm._32
            , &newFrame->baseXForm._42
            , &newFrame->baseXForm._13
            , &newFrame->baseXForm._23
            , &newFrame->baseXForm._33
            , &newFrame->baseXForm._43
            , &newFrame->baseXForm._14
            , &newFrame->baseXForm._24
            , &newFrame->baseXForm._34
            , &newFrame->baseXForm._44 );

    ParseFrame( *xmlChild, newFrame );
  }
}

void AnimationSet::AddAnimation( const wchar_t* name, const wchar_t* group, int start, int end )
{
  Animation animation;
  animation.name      = name;
  animation.group     = group;
  animation.startTick = start;
  animation.endTick   = end;

  animations.push_back( animation );
}

int AnimationSet::GetAnimationCount() const
{
  return int( animations.size() );
}

const wchar_t* AnimationSet::GetAnimationName( size_t index ) const
{
  return animations[ index ].name.data();
}

int AnimationSet::GetAnimationIndex( const wchar_t* name ) const
{
  for ( int ix = 0; ix < animations.size(); ix++ )
    if ( animations[ ix ].name == name )
      return ix;

  return -1;
}

int AnimationSet::GetAnimationLength( size_t index ) const
{
  return animations[ index ].endTick - animations[ index ].startTick;
}

void AnimationSet::EvaluateNode( FXMMATRIX parentXform, const Frame& frame, const Influences& infs, int tick, std::map< std::wstring, XMFLOAT4X4 >& nodes ) const
{
  XMMATRIX nodeTransform;

  auto iter = infs.find( frame.name );
  if ( iter == infs.end() )
  {
    nodeTransform = XMLoadFloat4x4( &frame.baseXForm );
  }
  else
  {
    const Influence& inf = iter->second;

    XMFLOAT3 scl( 1, 1, 1 );
    XMFLOAT3 pos( 0, 0, 0 );
    XMFLOAT4 rot( 0, 0, 0, 1 );

    if ( !inf.sclKeys.empty() )
    {
      auto sclIter = upper_bound( inf.sclKeys.begin(), inf.sclKeys.end(), tick, VectorKey::less );
      if ( sclIter == inf.sclKeys.end() )
        scl = inf.sclKeys.back().value;
      else
        scl = ( --sclIter )->value;
    }

    if ( !inf.posKeys.empty() )
    {
      auto posIter = upper_bound( inf.posKeys.begin(), inf.posKeys.end(), tick, VectorKey::less );
      if ( posIter == inf.posKeys.end() )
        pos = inf.posKeys.back().value;
      else
        pos = ( --posIter )->value;
    }

    if ( !inf.posKeys.empty() )
    {
      auto rotIter = upper_bound( inf.rotKeys.begin(), inf.rotKeys.end(), tick, QuatKey::less );
      if ( rotIter == inf.rotKeys.end() )
        rot = inf.rotKeys.back().value;
      else
        rot = ( --rotIter )->value;
    }

    auto s = XMMatrixScaling( scl.x, scl.y, scl.z );
    auto r = XMMatrixRotationQuaternion( XMLoadFloat4( &rot ) );
    auto t = XMMatrixTranslation( pos.x, pos.y, pos.z );
    nodeTransform = s * r * t;
  }

  auto _selfXform = nodeTransform * parentXform;

  XMStoreFloat4x4( &nodes[ frame.name ], _selfXform );

  for ( auto& child : frame.children )
    EvaluateNode( _selfXform, child, infs, tick, nodes );
}

void AnimationSet::Evaluate( int index, float rate, float tps, std::map< std::wstring, XMFLOAT4X4 >& nodes ) const
{
  auto& anim = animations[ index ];
  auto& infs = influences.find( anim.group )->second;
  auto  tick = anim.startTick + uint32_t( rate * tps );

  auto _identity = XMMatrixIdentity();
  for ( auto& frame : frames )
    EvaluateNode( _identity, frame, infs, tick, nodes );
}

AnimationSet::AnimationSet( std::vector< uint8_t > data )
{
  const uint8_t* raw = data.data();

  if ( *(uint32_t*)raw != ModelFeatures::animFCC )
    assert( false );

  raw += 4;

  auto features  = Read< uint32_t >( raw );
  auto animCount = Read< uint32_t >( raw );
  for ( uint32_t animIx = 0; animIx < animCount; animIx++ )
  {
    auto  animName = Read< std::wstring >( raw );
    auto& infs     = influences[ animName ];

    int boneCount = Read< uint32_t >( raw );
    for ( int boneIx = 0; boneIx < boneCount; boneIx++ )
    {
      auto  boneName = Read< std::wstring >( raw );
      auto& inf      = infs[ boneName ];
      inf.bone       = boneName;
      
      int sclCount = Read< uint32_t >( raw );
      inf.sclKeys.reserve( sclCount );
      for ( int sclIx = 0; sclIx < sclCount; sclIx++ )
      {
        auto tick = Read< uint32_t >( raw );
        XMFLOAT3 value;

        value.x = Read< float >( raw );
        value.y = Read< float >( raw );
        value.z = Read< float >( raw );

        inf.sclKeys.emplace_back( tick, value );
      }

      int rotCount = Read< uint32_t >( raw );
      inf.rotKeys.reserve( sclCount );
      for ( int rotIx = 0; rotIx < rotCount; rotIx++ )
      {
        auto tick = Read< uint32_t >( raw );
        XMFLOAT4 value;

        value.x = Read< float >( raw );
        value.y = Read< float >( raw );
        value.z = Read< float >( raw );
        value.w = Read< float >( raw );

        inf.rotKeys.emplace_back( tick, value );
      }

      int posCount = Read< uint32_t >( raw );
      inf.posKeys.reserve( sclCount );
      for ( int posIx = 0; posIx < posCount; posIx++ )
      {
        auto tick = Read< uint32_t >( raw );
        XMFLOAT3 value;

        value.x = Read< float >( raw );
        value.y = Read< float >( raw );
        value.z = Read< float >( raw );

        inf.posKeys.emplace_back( tick, value );
      }
    }
  }

  auto cursor   = raw - data.data();
  auto dataLeft = data.size() - cursor;

  tinyxml2::XMLDocument doc;
  doc.Parse( (const char*)raw, dataLeft );
  auto root = doc.RootElement();
  ParseFrame( *root, nullptr );
}
