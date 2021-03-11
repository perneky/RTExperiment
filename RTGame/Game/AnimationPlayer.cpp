#include "AnimationPlayer.h"
#include "AnimationSet.h"
#include "GameDefinition.h"
#include "Render/RenderManager.h"

void AnimationPlayer::SetCurrentAnimation( const wchar_t* name )
{
  currentAnimation      = name;
  currentAnimationIndex = animSet->GetAnimationIndex( name );
  ticksPerSecond        = 25;
  animationRate         = 0;
  animationLength       = animSet->GetAnimationLength( currentAnimationIndex ) / ticksPerSecond;
}

const wchar_t* AnimationPlayer::GetCurrentAnimation() const
{
  return currentAnimation.data();
}

int AnimationPlayer::GetAnimationCount() const
{
  if ( !animSet )
    return 0;

  return animSet->GetAnimationCount();
}

const wchar_t* AnimationPlayer::GetAnimationName( size_t index ) const
{
  if ( !animSet )
    return nullptr;

  return animSet->GetAnimationName( index );
}

void AnimationPlayer::Update( double timeElapsed )
{
  if ( !animSet || animationLength == 0 )
    return;

  animationRate = fmodf( animationRate + float( timeElapsed ), animationLength );
  animSet->Evaluate( currentAnimationIndex, animationRate, ticksPerSecond, nodeTransforms );
}

void AnimationPlayer::GetBoneTransforms( const std::vector< std::pair< std::wstring, XMFLOAT4X4 > >& boneNames, XMFLOAT4X4* boneXforms ) const
{
  if ( !animSet || animationLength == 0 )
    return;

  for ( int boneIx = 0; boneIx < boneNames.size(); boneIx++ )
  {
    auto& bone = boneNames[ boneIx ];
    auto  node = nodeTransforms.find( bone.first );
    if ( node != nodeTransforms.end() )
    {
      auto _boneXform = XMLoadFloat4x4( &bone.second  );
      auto _nodeXform = XMLoadFloat4x4( &node->second );
      XMStoreFloat4x4( &boneXforms[ boneIx ], XMMatrixTranspose( _boneXform * _nodeXform ) );
    }
  }

}

AnimationPlayer::AnimationPlayer( GUID animSetGUID )
{
  auto& animSets = gameDefinition.GetAnimationSets();
  auto iter = animSets.find( animSetGUID );
  if ( iter == animSets.end() )
    assert( false );

  auto& animSetDef = iter->second;

  auto Filler = [ = ]( AnimationSet* animSet )
  {
    for ( const auto& anim : animSetDef.animations )
      animSet->AddAnimation( anim.name.data(), anim.group.data(), anim.start, anim.end );
  };

  animSet = RenderManager::GetInstance().LoadAnimation( animSetDef.path.data(), Filler );

  std::vector< std::wstring > idleNames;
  for ( int animIx = 0; animIx < animSet->GetAnimationCount(); animIx++ )
  {
    auto animName = animSet->GetAnimationName( animIx );
    if ( tolower( animName[ 0 ] ) == 'i'
      && tolower( animName[ 1 ] ) == 'd'
      && tolower( animName[ 2 ] ) == 'l'
      && tolower( animName[ 3 ] ) == 'e' )
      idleNames.push_back( animName );
  }

  std::wstring startAnim;
  if ( idleNames.empty() )
    SetCurrentAnimation( animSet->GetAnimationName( 0 ) );
  else
    SetCurrentAnimation( idleNames[ rand() % idleNames.size() ].data() );
}
