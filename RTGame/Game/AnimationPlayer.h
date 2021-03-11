#pragma once

class AnimationSet;

class AnimationPlayer
{
public:
  AnimationPlayer( GUID animSetGUID );

  void           SetCurrentAnimation( const wchar_t* name );
  const wchar_t* GetCurrentAnimation() const;

  int            GetAnimationCount() const;
  const wchar_t* GetAnimationName ( size_t index ) const;

  void Update( double timeElapsed );
  void GetBoneTransforms( const std::vector< std::pair< std::wstring, XMFLOAT4X4 > >& boneNames, XMFLOAT4X4* boneXforms ) const;

private:
  GUID animSetGUID;

  std::shared_ptr< AnimationSet > animSet;

  std::wstring currentAnimation;
  int          currentAnimationIndex = 0;
  float        animationRate         = 0;
  float        animationLength       = 0;
  float        ticksPerSecond        = 0;

  std::map< std::wstring, XMFLOAT4X4 > nodeTransforms;
};
