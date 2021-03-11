#pragma once

#include "Render/ShaderStructuresNative.h"
#include "Types.h"

struct Resource;
struct CommandList;
struct Device;
struct RTBottomLevelAccelerator;

enum class AlphaModeCB : int;

class Mesh
{
  friend struct MeshHelper;

public:
  static std::unique_ptr< Mesh > CreateSkybox( CommandList& commandList, int materialIndex );
  static std::unique_ptr< Mesh > CreatePlane( CommandList& commandList, int materialIndex );
  static std::unique_ptr< Mesh > CreateFromMeshFile( CommandList& commandList, const wchar_t* filePath, MaterialTranslator materialTranslator );

  ~Mesh();

  void PrepareMeshParams( std::vector< MeshParamsCB >& allMeshParams
                        , CXMMATRIX worldTransform
                        , CXMMATRIX vTransform
                        , CXMMATRIX vpTransform
                        , CXMMATRIX prevWorldTransform
                        , CXMMATRIX prevVPTransform
                        , CXMMATRIX prevVTransform
                        , const std::set< int >& subsets
                        , const XMFLOAT4& randomValues
                        , float editorAddition ) const;

  void Render( Device& device
             , CommandList& commandList
             , int baseIndex
             , const std::set< int >& subsets
             , AlphaModeCB alphaMode
             , Resource* vertexBufferOverride
             , PrimitiveType primitiveType = PrimitiveType::TriangleList ) const;

  struct AccelData
  {
    RTBottomLevelAccelerator* accel    = nullptr;
    int                       rtIBSlot = -1;
    int                       rtVBSlot = -1;
  };

  AccelData GetRTAcceleratorForSubset( int subset );
  Resource& GetPackedMaterialIndices();

  bool IsSkin() const;
  bool HasTranslucent() const;
  int  GetTranslucentSidesMode() const;
  int  GetVertexCount() const;
  int  GetRTVertexSlot() const;
  int  GetRTIndexSlotForSubset( int subset ) const;

  const std::vector< std::pair< std::wstring, XMFLOAT4X4 > >& GetBoneNamesForSubset( int subset ) const;

  XMVECTOR CalcCenter() const;
  BoundingBox CalcBoundingBox() const;

  Resource& GetVertexBufferResource();
  Resource& GetIndexBufferResourceForSubset( int subset );

  std::pair< int, int > GetIndexRangeForSubset( int subset ) const;

  void Dispose( CommandList& commandList );

  float Pick( FXMMATRIX worldTransform, FXMVECTOR rayStart, FXMVECTOR rayDir ) const;

private:
  struct Batch;

  using IndexFormat = uint16_t;

  Mesh( CommandList& commandList, const RigidVertexFormat* vertices, int vertexCount, const IndexFormat* indices, int indexCount, std::vector< Batch > batches, const wchar_t* debugName );
  Mesh( CommandList& commandList, const SkinnedVertexFormat* vertices, int vertexCount, const IndexFormat* indices, int indexCount, std::vector< Batch > batches, const wchar_t* debugName );
  Mesh( CommandList& commandList, const SkyVertexFormat* vertices, int vertexCount, const IndexFormat* indices, int indexCount, std::vector< Batch > batches, const wchar_t* debugName );

  BoundingBox mergedAABB;

  std::unique_ptr< Resource > vertexBuffer;
  std::unique_ptr< Resource > rtVertexBuffer;

  std::vector< Batch > batches;

  std::unique_ptr< Resource > packedMaterialIndices;

  int rtVertexSlot = -1;

  int vertexCount = 0;

  bool isSkin = false;

  std::wstring debugName;
};