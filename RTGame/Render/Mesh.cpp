#include "Mesh.h"
#include "Device.h"
#include "Resource.h"
#include "ResourceDescriptor.h"
#include "RenderManager.h"
#include "CommandList.h"
#include "DescriptorHeap.h"
#include "Utils.h"
#include "RTBottomLevelAccelerator.h"
#include "ShaderStructuresNative.h"
#include "ModelFeatures.h"
#include "Common/Compression.h"
#include "Common/Files.h"

using namespace DirectX;

RTVertexFormat Convert( const RigidVertexFormat& v )
{
  RTVertexFormat rtv;
  rtv.normal.x      = XMConvertHalfToFloat( v.normal.x );
  rtv.normal.y      = XMConvertHalfToFloat( v.normal.y );
  rtv.normal.z      = XMConvertHalfToFloat( v.normal.z );
  rtv.tangent.x     = XMConvertHalfToFloat( v.tangent.x );
  rtv.tangent.y     = XMConvertHalfToFloat( v.tangent.y );
  rtv.tangent.z     = XMConvertHalfToFloat( v.tangent.z );
  rtv.bitangent.x   = XMConvertHalfToFloat( v.bitangent.x );
  rtv.bitangent.y   = XMConvertHalfToFloat( v.bitangent.y );
  rtv.bitangent.z   = XMConvertHalfToFloat( v.bitangent.z );
  rtv.texcoord.x    = XMConvertHalfToFloat( v.texcoord.x );
  rtv.texcoord.y    = XMConvertHalfToFloat( v.texcoord.y );
  rtv.materialIndex = 0;
  return rtv;
}

inline void ConvertToRigidVertexFormat( RigidVertexFormat& vtx, float x, float y, float z, float w, float nx, float ny, float nz, float tx, float ty, float tz, float bx, float by, float bz, float u, float v )
{
  vtx.position  = XMHALF4( vtx.position );
  vtx.normal    = XMHALF4( nx, ny, nz, 1 );
  vtx.tangent   = XMHALF4( tx, ty, tz, 1 );
  vtx.bitangent = XMHALF4( bx, by, bz, 1 );
  vtx.texcoord  = XMHALF2( vtx.texcoord );
}

inline void ConvertToSkyVertexFormat( SkyVertexFormat& vtx, float x, float y, float z )
{
  vtx.position = XMHALF4( x, y, z, 1 );
}

struct Mesh::Batch
{
  Batch() = default;
  Batch( int startIndex, int indexCount, int materialIndex, bool twoSided, AlphaModeCB alphaMode, const XMFLOAT3& aabbMin, const XMFLOAT3& aabbMax )
    : startIndex( startIndex ), indexCount( indexCount ), materialIndex( materialIndex ), twoSided( twoSided ), alphaMode( alphaMode ) 
  {
    BoundingBox::CreateFromPoints( aabb, XMLoadFloat3( &aabbMin ), XMLoadFloat3( &aabbMax ) );
  }
  ~Batch()
  {
    if ( rtSlot >= 0 )
      RenderManager::GetInstance().FreeRTIndexBufferSlot( rtSlot );
  }

  Batch( const Batch& ) = delete;
  Batch( Batch&& ) = default;

  int         startIndex;
  int         indexCount;
  int         materialIndex;
  bool        twoSided;
  AlphaModeCB alphaMode;
  BoundingBox aabb;

  std::unique_ptr< Resource > indexBuffer;

  std::unique_ptr< RTBottomLevelAccelerator > rtAccelerator;
  int                                         rtSlot = -1;

  std::vector< std::pair< std::wstring, XMFLOAT4X4 > > boneNames;
};

std::unique_ptr< Mesh > Mesh::CreateSkybox( CommandList& commandList, int materialIndex )
{
  std::vector< SkyVertexFormat   > vertices;
  std::vector< Mesh::IndexFormat > indices;

  auto n = -1.0f;
  auto p =  1.0f;

  vertices.resize( 8 );

  ConvertToSkyVertexFormat( vertices[ 0 ], n, p, n );
  ConvertToSkyVertexFormat( vertices[ 1 ], p, p, n );
  ConvertToSkyVertexFormat( vertices[ 2 ], n, n, n );
  ConvertToSkyVertexFormat( vertices[ 3 ], p, n, n );

  ConvertToSkyVertexFormat( vertices[ 4 ], n, n, p );
  ConvertToSkyVertexFormat( vertices[ 5 ], p, n, p );
  ConvertToSkyVertexFormat( vertices[ 6 ], n, p, p );
  ConvertToSkyVertexFormat( vertices[ 7 ], p, p, p );

  indices.resize( 20 );

  int index = 0;

  indices[ index++ ] = 1;
  indices[ index++ ] = 0;
  indices[ index++ ] = 3;
  indices[ index++ ] = 2;
  indices[ index++ ] = 5;
  indices[ index++ ] = 4;
  indices[ index++ ] = 7;
  indices[ index++ ] = 6;
  indices[ index++ ] = 1;
  indices[ index++ ] = 0;

  indices[ index++ ] = 0xffff;

  indices[ index++ ] = 7;
  indices[ index++ ] = 1;
  indices[ index++ ] = 5;
  indices[ index++ ] = 3;

  indices[ index++ ] = 0xffff;

  indices[ index++ ] = 0;
  indices[ index++ ] = 6;
  indices[ index++ ] = 2;
  indices[ index++ ] = 4;

  std::vector< Batch > batches;
  batches.emplace_back( 0, int( indices.size() ), materialIndex, true, AlphaModeCB::Opaque, XMFLOAT3( -1, -1, -1 ), XMFLOAT3( 1, 1, 1 ) );
  return std::unique_ptr< Mesh >( new Mesh( commandList, vertices.data(), int( vertices.size() ), indices.data(), int( indices.size() ), std::move( batches ), L"Skybox" ) );
}

std::unique_ptr< Mesh > Mesh::CreatePlane( CommandList& commandList, int materialIndex )
{
  std::vector< RigidVertexFormat > vertices;
  std::vector< Mesh::IndexFormat > indices;

  vertices.resize( 4 );
  ConvertToRigidVertexFormat( vertices[ 0 ], -1, 0,  1, 1,   0, 1, 0,   1, 0, 0,   0, 0, 1,   0, 1 );
  ConvertToRigidVertexFormat( vertices[ 1 ],  1, 0,  1, 1,   0, 1, 0,   1, 0, 0,   0, 0, 1,   1, 1 );
  ConvertToRigidVertexFormat( vertices[ 2 ], -1, 0, -1, 1,   0, 1, 0,   1, 0, 0,   0, 0, 1,   0, 0 );
  ConvertToRigidVertexFormat( vertices[ 3 ],  1, 0, -1, 1,   0, 1, 0,   1, 0, 0,   0, 0, 1,   1, 0 );

  indices.resize( 6 );
  indices[ 0 ] = 0;
  indices[ 1 ] = 1;
  indices[ 2 ] = 2;
  indices[ 3 ] = 1;
  indices[ 4 ] = 3;
  indices[ 5 ] = 2;

  std::vector< Batch > batches;
  batches.emplace_back( 0, int( indices.size() ), materialIndex, true, AlphaModeCB::Opaque, XMFLOAT3( -1, 0, -1 ), XMFLOAT3( 1, 0, 1 ) );
  return std::unique_ptr< Mesh >( new Mesh( commandList, vertices.data(), int( vertices.size() ), indices.data(), int( indices.size() ), std::move( batches ), L"Plane" ) );
}

std::unique_ptr< Mesh > Mesh::CreateFromMeshFile( CommandList& commandList, const wchar_t* filePath, MaterialTranslator materialTranslator )
{
  auto data = ReadFileToMemory( filePath );

  bool isSkin = false;

  const uint8_t* raw = data.data();
  if ( *(uint32_t*)raw == ModelFeatures::modelFCC )
  {
    raw += sizeof uint32_t;
    uint32_t features = Read< uint32_t >( raw );

    if ( ( features & ModelFeatures::Skin ) == ModelFeatures::Skin )
      isSkin = true;
    if ( ( features & ModelFeatures::Index32 ) == ModelFeatures::Index32 )
      assert( false );
  }

  int sectionCount = Read< uint8_t >( raw );

  int startIndex       = 0;
  int totalVertexCount = 0;
  int totalIndexCount  = 0;

  std::vector< Batch > batches;

  for ( int sectionIx = 0; sectionIx < sectionCount; sectionIx++ )
  {
    int materialIx  = Read< uint8_t  >( raw );
    int vertexCount = Read< uint32_t >( raw );
    int indexCount  = Read< uint32_t >( raw ) * 3;

    int         batchMaterialIndex;
    bool        batchTwoSided;
    AlphaModeCB batchAlphaMode;

    materialTranslator( sectionIx, materialIx, batchMaterialIndex, batchTwoSided, batchAlphaMode );

    XMFLOAT3 aabbMin, aabbMax;
    aabbMin.x = Read< float >( raw );
    aabbMin.y = Read< float >( raw );
    aabbMin.z = Read< float >( raw );
    aabbMax.x = Read< float >( raw );
    aabbMax.y = Read< float >( raw );
    aabbMax.z = Read< float >( raw );

    batches.emplace_back( startIndex, indexCount, batchMaterialIndex, batchTwoSided, batchAlphaMode, aabbMin, aabbMax );

    if ( isSkin )
    {
      auto numBones = Read< uint8_t >( raw );
      for ( size_t boneIx = 0; boneIx < numBones; boneIx++ )
      {
        std::pair< std::wstring, XMFLOAT4X4 > bone;
        bone.first  = Read< std::wstring >( raw );
        bone.second = Read< XMFLOAT4X4 >( raw );
        XMStoreFloat4x4( &bone.second, XMMatrixTranspose( XMLoadFloat4x4( &bone.second ) ) );
        batches.back().boneNames.emplace_back( std::move( bone ) );
      }
    }

    startIndex       += indexCount;
    totalVertexCount += vertexCount;
    totalIndexCount  += indexCount;
  }

  const void* vertexData = raw;
  const void*  indexData = raw + totalVertexCount * ( isSkin ? sizeof( SkinnedVertexFormat ) : sizeof( RigidVertexFormat ) );

  if ( isSkin )
    return std::unique_ptr< Mesh >( new Mesh( commandList, (const SkinnedVertexFormat*)vertexData, totalVertexCount, (const IndexFormat*)indexData, totalIndexCount, std::move( batches ), filePath ) );
  else
    return std::unique_ptr< Mesh >( new Mesh( commandList, (const RigidVertexFormat*)vertexData, totalVertexCount, (const IndexFormat*)indexData, totalIndexCount, std::move( batches ), filePath ) );
}

struct MeshHelper
{
  template< typename V, typename I >
  static void GenericCreateMesh( CommandList& commandList, const V* vertices, int vertexCount, const I* indices, int indexCount, std::vector< Mesh::Batch >& batches, Mesh& mesh, bool needsRT, bool isSkin )
  {
    auto& device = RenderManager::GetInstance().GetDevice();

    mesh.vertexBuffer = CreateBufferFromData( vertices, vertexCount, ResourceType::VertexBuffer, device, commandList, L"MeshVB" );
    commandList.ChangeResourceState( *mesh.vertexBuffer, ResourceStateBits::VertexOrConstantBuffer | ResourceStateBits::NonPixelShaderInput );

    mesh.batches.swap( batches );

    for ( auto& batch : mesh.batches )
      batch.indexBuffer = CreateBufferFromData( indices + batch.startIndex, batch.indexCount, ResourceType::IndexBuffer, device, commandList, L"MeshIB" );

    mesh.vertexCount = vertexCount;
    mesh.isSkin      = isSkin;

    if ( needsRT )
    {
      mesh.rtVertexSlot = RenderManager::GetInstance().ReserveRTVertexBufferSlot();

      if ( !isSkin )
      {
        std::vector< RTVertexFormat > rtVertices;
        rtVertices.reserve( vertexCount );
        auto mvtx = reinterpret_cast< const RigidVertexFormat* >( vertices );
        for ( int vtx = 0; vtx < vertexCount; ++vtx, ++mvtx )
          rtVertices.emplace_back( Convert( *mvtx ) );

        for ( auto& batch : mesh.batches )
          for ( int ix = batch.startIndex; ix < batch.startIndex + batch.indexCount; ++ix )
            rtVertices[ indices[ ix ] ].materialIndex = batch.materialIndex;

        mesh.rtVertexBuffer = CreateBufferFromData( rtVertices.data(), int( rtVertices.size() ), ResourceType::ConstantBuffer, device, commandList, L"MeshVBRT" );

        auto vbIndexDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, mesh.rtVertexSlot, *mesh.rtVertexBuffer, sizeof( RTVertexFormat ) );
        mesh.rtVertexBuffer->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( vbIndexDesc ) );

        for ( auto& batch : mesh.batches )
        {
          batch.rtSlot = RenderManager::GetInstance().ReserveRTIndexBufferSlot();

          auto ibIndexDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, batch.rtSlot, *batch.indexBuffer, sizeof( I ) );
          batch.indexBuffer->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( ibIndexDesc ) );

          commandList.ChangeResourceState( *batch.indexBuffer, ResourceStateBits::IndexBuffer | ResourceStateBits::NonPixelShaderInput | ResourceStateBits::PixelShaderInput );

          batch.rtAccelerator = device.CreateRTBottomLevelAccelerator( commandList, *mesh.vertexBuffer, vertexCount, 16, sizeof( RigidVertexFormat ), *batch.indexBuffer, 16, batch.indexCount, false, false );
        }
      }
      else
      {
        std::vector< uint32_t > packedIndices( vertexCount, 0 );
        for ( auto& batch : mesh.batches )
        {
          for ( int ix = batch.startIndex; ix < batch.startIndex + batch.indexCount; ++ix )
            packedIndices[ indices[ ix ] ] = batch.materialIndex;

          batch.rtSlot = RenderManager::GetInstance().ReserveRTIndexBufferSlot();

          auto ibIndexDesc = device.GetShaderResourceHeap().RequestDescriptor( device, ResourceDescriptorType::ShaderResourceView, batch.rtSlot, *batch.indexBuffer, sizeof( I ) );
          batch.indexBuffer->AttachResourceDescriptor( ResourceDescriptorType::ShaderResourceView, std::move( ibIndexDesc ) );

          commandList.ChangeResourceState( *batch.indexBuffer, ResourceStateBits::IndexBuffer | ResourceStateBits::NonPixelShaderInput | ResourceStateBits::PixelShaderInput );
        }

        mesh.packedMaterialIndices = CreateBufferFromData( packedIndices.data(), int( packedIndices.size() ), ResourceType::ConstantBuffer, device, commandList, L"MeshMaterials" );

        commandList.ChangeResourceState( *mesh.vertexBuffer, ResourceStateBits::NonPixelShaderInput | ResourceStateBits::VertexOrConstantBuffer );
      }
    }

    mesh.mergedAABB = mesh.batches.front().aabb;
    for ( auto& batch : mesh.batches )
      BoundingBox::CreateMerged( mesh.mergedAABB, mesh.mergedAABB, batch.aabb );
  }
};

Mesh::Mesh( CommandList& commandList, const RigidVertexFormat* vertices, int vertexCount, const IndexFormat* indices, int indexCount, std::vector< Batch > batches, const wchar_t* debugName )
  : debugName( debugName )
{
  MeshHelper::GenericCreateMesh( commandList, vertices, vertexCount, indices, indexCount, batches, *this, true, false );
}

Mesh::Mesh( CommandList& commandList, const SkyVertexFormat* vertices, int vertexCount, const IndexFormat* indices, int indexCount, std::vector< Batch > batches, const wchar_t* debugName )
  : debugName( debugName )
{
  MeshHelper::GenericCreateMesh( commandList, vertices, vertexCount, indices, indexCount, batches, *this, false, false );
}

Mesh::Mesh( CommandList& commandList, const SkinnedVertexFormat* vertices, int vertexCount, const IndexFormat* indices, int indexCount, std::vector< Batch > batches, const wchar_t* debugName )
  : debugName( debugName )
{
  MeshHelper::GenericCreateMesh( commandList, vertices, vertexCount, indices, indexCount, batches, *this, true, true );
}

Mesh::~Mesh()
{
  if ( rtVertexSlot >= 0 )
    RenderManager::GetInstance().FreeRTVertexBufferSlot( rtVertexSlot );
}

void Mesh::PrepareMeshParams( std::vector< MeshParamsCB >& allMeshParams
                            , CXMMATRIX worldTransform
                            , CXMMATRIX vTransform
                            , CXMMATRIX vpTransform
                            , CXMMATRIX prevWorldTransform
                            , CXMMATRIX prevVPTransform
                            , CXMMATRIX prevVTransform
                            , const std::set< int >& subsets
                            , const XMFLOAT4& randomValues
                            , float editorAddition ) const
{
  MeshParamsCB meshParams;
  XMStoreFloat4x4( &meshParams.worldTransform,     worldTransform );
  XMStoreFloat4x4( &meshParams.wvpTransform,       XMMatrixMultiply( worldTransform, vpTransform ) );
  XMStoreFloat4x4( &meshParams.wvTransform,        XMMatrixMultiply( worldTransform, vTransform ) );
  XMStoreFloat4x4( &meshParams.prevWorldTransform, prevWorldTransform );
  XMStoreFloat4x4( &meshParams.prevWVPTransform,   XMMatrixMultiply( prevWorldTransform, prevVPTransform ) );
  XMStoreFloat4x4( &meshParams.prevWVTransform,    XMMatrixMultiply( prevWorldTransform, prevVTransform ) );
  meshParams.randomValues   = randomValues;
  meshParams.editorAddition = editorAddition;

  for ( auto subset : subsets )
  {
    auto& batch = batches[ subset ];

    meshParams.materialIndex = batch.materialIndex;
    allMeshParams.emplace_back( meshParams );
  }
}

void Mesh::Render( Device& device
                 , CommandList& commandList
                 , int baseIndex
                 , const std::set< int >& subsets
                 , AlphaModeCB alphaMode
                 , Resource* vertexBufferOverride
                 , PrimitiveType primitiveType ) const
{
  bool needDraw = false;
  for ( auto& subset : subsets )
  {
    auto& batch = batches[ subset ];
    if ( batch.alphaMode == alphaMode )
    {
      needDraw = true;
      break;
    }
  }

  if ( !needDraw )
    return;

  commandList.BeginEvent( 2315, L"Mesh::Render(%s)", debugName.empty() ? L"null" : debugName.data() );

  commandList.SetVertexBuffer( vertexBufferOverride ? *vertexBufferOverride : *vertexBuffer );

  int listIndex = -1;
  for ( auto subset : subsets )
  {
    listIndex++;

    auto& batch = batches[ subset ];

    if ( batch.alphaMode != alphaMode )
      continue;

    commandList.SetIndexBuffer( *batch.indexBuffer );

    int paramIndex = baseIndex + listIndex;
    commandList.SetConstantValues( 0, paramIndex );
    commandList.SetPrimitiveType( primitiveType );
    commandList.DrawIndexed( batch.indexCount, 1, 0 );
  }

  commandList.EndEvent();
}

Mesh::AccelData Mesh::GetRTAcceleratorForSubset( int subset )
{
  assert( subset < batches.size() );

  AccelData data;
  data.accel    = batches[ subset ].rtAccelerator.get();
  data.rtIBSlot = batches[ subset ].rtSlot;
  data.rtVBSlot = rtVertexSlot;
  return data;
}

Resource& Mesh::GetPackedMaterialIndices()
{
  return *packedMaterialIndices;
}

bool Mesh::IsSkin() const
{
  return isSkin;
}

bool Mesh::HasTranslucent() const
{
  for ( auto& batch : batches )
    if ( batch.alphaMode == AlphaModeCB::Translucent )
      return true;

  return false;
}

int Mesh::GetTranslucentSidesMode() const
{
  int mode = 0;

  for ( auto& batch : batches )
  {
    if ( batch.alphaMode == AlphaModeCB::Translucent )
    {
      int sides = batch.twoSided ? 2 : 1;
      if ( mode == 0 )
        mode = sides;
      else
        assert( mode == sides );
    }
  }

  return std::max( mode, 1 );
}

int Mesh::GetVertexCount() const
{
  return vertexCount;
}

int Mesh::GetRTVertexSlot() const
{
  return rtVertexSlot;
}

int Mesh::GetRTIndexSlotForSubset( int subset ) const
{
  assert( subset < batches.size() );
  return batches[ subset ].rtSlot;
}

const std::vector< std::pair< std::wstring, XMFLOAT4X4 > >& Mesh::GetBoneNamesForSubset( int subset ) const
{
  assert( subset < batches.size() );
  return batches[ subset ].boneNames;
}

XMVECTOR Mesh::CalcCenter() const
{
  if ( batches.empty() )
    return XMVectorSet( 0, 0, 0, 1 );

  float count = float( batches.size() );

  auto center = XMVectorSet( 0, 0, 0, 1 );
  for ( auto& batch : batches )
    center = XMVectorAdd( XMLoadFloat3( &batch.aabb.Center ), center );
  return XMVectorDivide( center, XMVectorSet( count, count, count, 1 ) );
}

BoundingBox Mesh::CalcBoundingBox() const
{
  return mergedAABB;
}

Resource& Mesh::GetVertexBufferResource()
{
  return *vertexBuffer;
}

Resource& Mesh::GetIndexBufferResourceForSubset( int subset )
{
  assert( subset < batches.size() );
  return *batches[ subset ].indexBuffer;
}

std::pair< int, int > Mesh::GetIndexRangeForSubset( int subset ) const
{
  assert( subset < batches.size() );
  auto& batch = batches[ subset ];
  return { batch.startIndex, batch.indexCount };
}

void Mesh::Dispose( CommandList& commandList )
{
  commandList.HoldResource( std::move( vertexBuffer ) );
  commandList.HoldResource( std::move( rtVertexBuffer ) );
  commandList.HoldResource( std::move( packedMaterialIndices ) );
  for ( auto& batch : batches )
  {
    batch.indexBuffer->RemoveAllResourceDescriptors();
    commandList.HoldResource( std::move( batch.indexBuffer ) );
  }
}

float Mesh::Pick( FXMMATRIX worldTransform, FXMVECTOR rayStart, FXMVECTOR rayDir ) const
{
  float dist = FLT_MAX;
  for ( auto& batch : batches )
  {
    float ld;
    BoundingOrientedBox orientedBox, transformedOrientedBox;
    BoundingOrientedBox::CreateFromBoundingBox( orientedBox, batch.aabb );
    orientedBox.Transform( transformedOrientedBox, worldTransform );
    if ( !transformedOrientedBox.Contains( rayStart )
      && transformedOrientedBox.Intersects( rayStart, rayDir, ld ) )
      dist = std::min( dist, ld );
  }

  if ( dist == FLT_MAX )
    return -1;

  return dist;
}
