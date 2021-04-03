#pragma once

namespace tinyxml2 { class XMLNode; }

enum class EntityType
{
  Model,
  DirectionalLight,
  Sky,
  Camera,
};

struct CommandList;

class Entity
{
public:
  virtual ~Entity();

  XMVECTOR GetPosition() const;
  XMVECTOR GetRotation() const;
  XMVECTOR GetScale   () const;

  XMMATRIX GetWorldTransform() const;

  void SetPosition( FXMVECTOR v );
  void SetRotation( FXMVECTOR v );
  void SetScale   ( FXMVECTOR v );

  virtual float Pick( FXMVECTOR rayStart, FXMVECTOR rayDir ) const { return -1; }

  virtual void Serialize  ( uint32_t id, tinyxml2::XMLNode& node );
  virtual void Deserialize( tinyxml2::XMLNode& node );

  virtual void Update( CommandList& commandList, bool mayUseGPU, double timeElapsed ) {}

  virtual std::unique_ptr< Entity > Duplicate( CommandList& commandList ) = 0;

protected:
  void RecalcWorldTransform();

  virtual void TransformChanged() {}

  XMFLOAT3 position;
  XMFLOAT3 rotation;
  XMFLOAT3 scale = XMFLOAT3( 1, 1, 1 );

  XMFLOAT4X4 worldTransform;
};