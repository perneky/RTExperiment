#include "stdafx.h"
#include "../External/tinyxml2/tinyxml2.cpp"
#include "../RTGame/Render/ModelFeatures.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define REAQ( m ) { printf_s( m "\n" ); return false; }

#pragma pack( push, 1 )
struct Vertex
{
  Float16::Type position [ 4 ];
  Float16::Type normal   [ 4 ];
  Float16::Type tangent  [ 4 ];
  Float16::Type bitangent[ 4 ];
  Float16::Type texcoord [ 2 ];
};
struct AnimatedVertex
{
  Float16::Type position   [ 4 ];
  Float16::Type normal     [ 4 ];
  Float16::Type tangent    [ 4 ];
  Float16::Type bitangent  [ 4 ];
  Float16::Type texcoord   [ 2 ];
  uint8_t       boneIndices[ 4 ];
  uint8_t       boneWeights[ 4 ];
};
struct AABB
{
  float min[ 3 ] = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
  float max[ 3 ] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
};
#pragma pack( pop )

void Compress( Float16::Type* c, const aiVector2D& u, float scale = 1 )
{
  Float16::Convert( &c[ 0 ], u.x * scale );
  Float16::Convert( &c[ 1 ], u.y * scale );
}

void Compress( Float16::Type* c, const aiVector3D& u, float scale = 1 )
{
  Float16::Convert( &c[ 0 ], u.x * scale );
  Float16::Convert( &c[ 1 ], u.y * scale );
  Float16::Convert( &c[ 2 ], u.z * scale );
  Float16::Convert( &c[ 3 ], 1.0f );
}

using BoneIndices = uint8_t[ 4 ];
using BoneWeights = float[ 4 ];
struct BoneLink
{
  BoneLink()
  {
    memset( indices, 0, sizeof indices );
    memset( weights, 0, sizeof weights );
  }

  void Add( unsigned i, float w )
  {
    for ( int slot = 0; slot < 4; slot++ )
    {
      if ( weights[ slot ] == 0 )
      {
        indices[ slot ] = uint8_t( i );
        weights[ slot ] = w;
        return;
      }
    }

    float   minW = weights[ 0 ];
    uint8_t minI = 0;
    for ( int slot = 1; slot < 4; slot++ )
    {
      if ( weights[ slot ] < minW )
      {
        minW = weights[ slot ];
        minI = indices[ slot ];
      }
    }

    if ( w > minW )
    {
      indices[ minI ] = uint8_t( i );
      weights[ minI ] = w;
    }
  }

  void Normalize()
  {
    float l = 0;
    for ( int slot = 0; slot < 4; slot++ )
      l += weights[ slot ];

    if ( l == 0 )
      return;

    for ( int slot = 0; slot < 4; slot++ )
      weights[ slot ] = weights[ slot ] / l;
  }

  BoneIndices indices;
  BoneWeights weights;
};

template< typename T >
static void Write( FILE* f, T v )
{
  fwrite( &v, sizeof v, 1, f );
}

void ConvertTexture( const char* srcFile, const char* dstLocation, const char* fmt )
{
  auto fileName = PathFindFileNameA( srcFile );

  std::string dstFile( dstLocation );
  auto lastDash = dstFile.rfind( '\\' );
  dstFile.resize( lastDash + 1 );
  dstFile += fileName;

  auto lastDot = dstFile.rfind( '.' );
  dstFile.resize( lastDot );
  dstFile += ".dds";

  if ( strcmp( fmt, "DXT" ) == 0 )
  {
    int components = 0;
    if ( stbi_info( srcFile, nullptr, nullptr, &components ) )
      fmt = ( components == 4 ) ? "BC2" : "BC1";
    else
      return;
  }

  std::string command;
  command += R"("c:\Users\laszl\AppData\Local\Compressonator\bin\CLI\compressonatorcli.exe" -silent -noprogress -EncodeWith GPU -miplevels 20 )";
  command += "-fd ";
  command += fmt;
  command += " ";
  command += srcFile;
  command += " ";
  command += dstFile;
  system( command.data() );
}

bool IsLowerLOD( const aiString& nameAI )
{
  if ( nameAI.length <= 4 )
    return false;

  std::string name = std::string( nameAI.C_Str(), nameAI.length );
  std::transform( name.begin(), name.end(), name.begin(), tolower );

  char lodt[ 5 ] = "lod0";
  for ( int lodIx = 1; lodIx < 10; ++lodIx )
  {
    lodt[ 3 ] = '0' + lodIx;
    if ( name.rfind( lodt ) == name.size() - 4 )
      return true;
  }

  return false;
}

std::string NormalizeEntityName( const aiNode* node )
{
  std::string name = std::string( node->mName.C_Str(), node->mName.length );
  std::transform( name.begin(), name.end(), name.begin(), tolower );

  if ( name.size() > 5 && name.rfind( "_lod0" ) == name.size() - 5 )
    return std::string( node->mName.C_Str(), node->mName.length - 5 );
  else
    return std::string( node->mName.C_Str(), node->mName.length );
}

bool IsLowerLOD( aiMesh* mesh )
{
  return IsLowerLOD( mesh->mName );
}

bool IsLowerLOD( aiNode* node )
{
  return IsLowerLOD( node->mName );
}

bool Validate( aiMesh* mesh, unsigned meshIx )
{
  aiString    meshName  = mesh->mName;
  const char* meshNameC = meshName.C_Str();
  if ( !mesh->HasPositions() )
  {
    printf_s( "Mesh (%d) has no positions: %s\n", meshIx, meshNameC );
    return false;
  }
  if ( !mesh->HasFaces() )
  {
    printf_s( "Mesh (%d) has no faces: %s\n", meshIx, meshNameC );
    return false;
  }
  if ( !mesh->HasNormals() )
  {
    printf_s( "Mesh (%d) has no normals: %s\n", meshIx, meshNameC );
    return false;
  }
  if ( mesh->GetNumUVChannels() < 1 )
  {
    printf_s( "Mesh (%d) has no texture coordinates: %s\n", meshIx, meshNameC );
  }
  if ( mesh->GetNumUVChannels() > 1 )
  {
    printf_s( "Mesh (%d) has more than one texture coordinates: %s\n", meshIx, meshNameC );
  }
  if ( !mesh->HasTangentsAndBitangents() )
  {
    printf_s( "Mesh (%d) has no tangents: %s\n", meshIx, meshNameC );
  }

  return true;
}

bool HasBoneInTree( const aiNode* node, const std::set< std::string >& bones )
{
  if ( bones.find( node->mName.C_Str() ) != bones.end() )
    return true;

  for ( unsigned childIx = 0; childIx < node->mNumChildren; childIx++ )
    if ( HasBoneInTree( node->mChildren[ childIx ], bones ) )
      return true;

  return false;
}

void ExportFrame( const aiNode* sceneNode, XMLNode* xmlNode, const std::set< std::string >& bones )
{
  if ( !HasBoneInTree( sceneNode, bones ) )
    return;

  XMLElement* frameNode = (XMLElement*)xmlNode->InsertEndChild( xmlNode->GetDocument()->NewElement( "Frame" ) );
  frameNode->SetAttribute( "name", sceneNode->mName.C_Str() );
  
  char t[ 1024 ];
  sprintf_s( t, "%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f"
           , sceneNode->mTransformation.a1
           , sceneNode->mTransformation.a2
           , sceneNode->mTransformation.a3
           , sceneNode->mTransformation.a4
           , sceneNode->mTransformation.b1
           , sceneNode->mTransformation.b2
           , sceneNode->mTransformation.b3
           , sceneNode->mTransformation.b4
           , sceneNode->mTransformation.c1
           , sceneNode->mTransformation.c2
           , sceneNode->mTransformation.c3
           , sceneNode->mTransformation.c4
           , sceneNode->mTransformation.d1
           , sceneNode->mTransformation.d2
           , sceneNode->mTransformation.d3
           , sceneNode->mTransformation.d4 );
  frameNode->SetAttribute( "xform", t );

  for ( unsigned nodeIx = 0; nodeIx < sceneNode->mNumChildren; nodeIx++ )
    ExportFrame( sceneNode->mChildren[ nodeIx ], frameNode, bones );
}

aiString Extract( const char* name, int, int )
{
  return aiString( name );
}

bool FindTransform( const aiScene* scene, const aiNode* node, const aiMesh* mesh, aiMatrix3x3& result )
{
  for ( unsigned meshIx = 0; meshIx < node->mNumMeshes; meshIx++ )
    if ( scene->mMeshes[ node->mMeshes[ meshIx ] ] == mesh )
    {
      result = aiMatrix3x3( node->mTransformation );
      return true;
    }

  for ( unsigned childIx = 0; childIx < node->mNumChildren; childIx++ )
    if ( FindTransform( scene, node->mChildren[ childIx ], mesh, result ) )
      return true;

  return false;
}

template< typename index_t >
bool ConvertFile( float scale, const char* srcFile, const char* dstFile, XMLNode* exml, XMLNode* mxml, const char* basePath, bool noTexture, bool rotate90 )
{
  std::string srcDir( srcFile );
  auto lastDash = srcDir.rfind( '\\' );
  srcDir.resize( lastDash );

  SetCurrentDirectoryA( srcDir.data() );

  Assimp::Importer importer;

  unsigned flags = aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality;
  const aiScene* scene = importer.ReadFile( srcFile, flags );

  if ( !scene )
  {
    printf_s( "Failed to open %s.\n", srcFile );
    return false;
  }

  if ( !scene->HasMeshes() )
    REAQ( "Opened scene has no meshes." );

  if ( strstr( srcFile, "NewspaperBox.fbx" ) != nullptr )
    scene->mMeshes[ 0 ]->mName.data[ scene->mMeshes[ 0 ]->mName.length - 1 ] = '0';

  if ( strstr( srcFile, "BusStop.fbx" ) != nullptr )
    scene->mMeshes[ 0 ]->mName.data[ scene->mMeshes[ 0 ]->mName.length - 1 ] = '0';

  std::map< int, int > meshRemap;

  int numMeshes = 0;
  for ( unsigned meshIx = 0; meshIx < scene->mNumMeshes; meshIx++ )
  {
    aiMesh* mesh = scene->mMeshes[ meshIx ];
    if ( !Validate( mesh, meshIx ) )
      continue;

    if ( IsLowerLOD( mesh ) )
      continue;

    meshRemap[ meshIx ] = numMeshes;

    numMeshes++;
  }

  if ( numMeshes == 0 )
    REAQ( "Opened scene has no valid or non low level LOD meshes." );

  FILE* outFile = nullptr;
  if ( fopen_s( &outFile, dstFile, "wb" ) )
  {
    printf_s( "Failed to open %s for writing.\n", dstFile );
    return false;
  }

  Write( outFile, ModelFeatures::modelFCC );

  uint32_t features = 0;

  for ( unsigned meshIx = 0; meshIx < scene->mNumMeshes; meshIx++ )
    if ( scene->mMeshes[ meshIx ]->HasBones() )
      features |= ModelFeatures::Skin;

  if ( sizeof index_t > 2 )
    features |= ModelFeatures::Index32;

  Write( outFile, features );

  std::vector< uint32_t > vertexCounts;
  std::set< std::string > boneNames;

  Write( outFile, uint8_t( numMeshes ) );
  for ( unsigned meshIx = 0; meshIx < scene->mNumMeshes; meshIx++ )
  {
    aiMesh* mesh = scene->mMeshes[ meshIx ];
    if ( !Validate( mesh, meshIx ) )
      continue;

    if ( IsLowerLOD( mesh ) )
      continue;

    aiMatrix3x3 xform;
    FindTransform( scene, scene->mRootNode, mesh, xform );

    Write( outFile, uint8_t ( mesh->mMaterialIndex ) );
    Write( outFile, uint32_t( mesh->mNumVertices   ) );
    Write( outFile, uint32_t( mesh->mNumFaces      ) );

    AABB aabb;
    for ( unsigned vtxIx = 0; vtxIx < mesh->mNumVertices; vtxIx++ )
    {
      aiVector3D v = mesh->mVertices[ vtxIx ];

      if ( !mesh->HasBones() )
        v *= xform;

      aabb.min[ 0 ] = std::min( aabb.min[ 0 ], v.x * scale );
      aabb.min[ 1 ] = std::min( aabb.min[ 1 ], v.y * scale );
      aabb.min[ 2 ] = std::min( aabb.min[ 2 ], v.z * scale );

      aabb.max[ 0 ] = std::max( aabb.max[ 0 ], v.x * scale );
      aabb.max[ 1 ] = std::max( aabb.max[ 1 ], v.y * scale );
      aabb.max[ 2 ] = std::max( aabb.max[ 2 ], v.z * scale );
    }
    Write( outFile, aabb );

    if ( mesh->HasBones() )
    {
      Write( outFile, uint8_t( mesh->mNumBones ) );
      for ( unsigned boneIx = 0; boneIx < mesh->mNumBones; boneIx++ )
      {
        const aiBone* bone = mesh->mBones[ boneIx ];
        boneNames.insert( bone->mName.C_Str() );
        fwrite( bone->mName.C_Str(), 1, bone->mName.length + 1, outFile );
        fwrite( &bone->mOffsetMatrix.a1, sizeof( float ), 16, outFile );
      }
    }

    vertexCounts.push_back( mesh->mNumVertices );
  }

  for ( unsigned meshIx = 0; meshIx < scene->mNumMeshes; meshIx++ )
  {
    aiMesh* mesh = scene->mMeshes[ meshIx ];

    if ( !Validate( mesh, meshIx ) )
      continue;

    if ( IsLowerLOD( mesh ) )
      continue;

    aiMatrix3x3 xform;
    FindTransform( scene, scene->mRootNode, mesh, xform );

    printf_s( "Mesh name: %s, submesh: %d, material: %d\n", mesh->mName.C_Str(), meshIx, mesh->mMaterialIndex );

    if ( mesh->HasBones() )
    {
      std::vector< BoneLink > boneLinks( mesh->mNumVertices );
      for ( unsigned boneIx = 0; boneIx < mesh->mNumBones; boneIx++ )
      {
        const aiBone* bone = mesh->mBones[ boneIx ];

        for ( unsigned weightIx = 0; weightIx < bone->mNumWeights; weightIx++ )
        {
          const aiVertexWeight& weight = bone->mWeights[ weightIx ];
          boneLinks[ weight.mVertexId ].Add( boneIx, weight.mWeight );
        }
      }

      for ( auto& boneLink : boneLinks )
        boneLink.Normalize();

      aiVector3D zeroVector( 0, 0, 0 );

      AnimatedVertex vtx;
      for ( unsigned vtxIx = 0; vtxIx < mesh->mNumVertices; vtxIx++ )
      {
        const aiVector3D& v = mesh->mVertices[ vtxIx ];
        const aiVector3D& n = mesh->mNormals [ vtxIx ];
        const aiVector3D& t = mesh->mTangents           ? mesh->mTangents          [ vtxIx ] : zeroVector;
        const aiVector3D& b = mesh->mBitangents         ? mesh->mBitangents        [ vtxIx ] : zeroVector;
        const aiVector3D& x = mesh->mTextureCoords[ 0 ] ? mesh->mTextureCoords[ 0 ][ vtxIx ] : zeroVector;

        Compress( vtx.position,  v, scale );
        Compress( vtx.normal,    n );
        Compress( vtx.tangent,   t );
        Compress( vtx.bitangent, b );

        Float16::Convert( &vtx.texcoord[ 0 ], x.x );
        Float16::Convert( &vtx.texcoord[ 1 ], x.y );

        vtx.boneIndices[ 0 ] = boneLinks[ vtxIx ].indices[ 0 ];
        vtx.boneIndices[ 1 ] = boneLinks[ vtxIx ].indices[ 1 ];
        vtx.boneIndices[ 2 ] = boneLinks[ vtxIx ].indices[ 2 ];
        vtx.boneIndices[ 3 ] = boneLinks[ vtxIx ].indices[ 3 ];

        vtx.boneWeights[ 0 ] = uint8_t( boneLinks[ vtxIx ].weights[ 0 ] * 255 );
        vtx.boneWeights[ 1 ] = uint8_t( boneLinks[ vtxIx ].weights[ 1 ] * 255 );
        vtx.boneWeights[ 2 ] = uint8_t( boneLinks[ vtxIx ].weights[ 2 ] * 255 );
        vtx.boneWeights[ 3 ] = uint8_t( boneLinks[ vtxIx ].weights[ 3 ] * 255 );

        auto weightSum = int( vtx.boneWeights[ 0 ] ) 
                       + int( vtx.boneWeights[ 1 ] )
                       + int( vtx.boneWeights[ 2 ] )
                       + int( vtx.boneWeights[ 3 ] );

        assert( weightSum > 250 && weightSum < 260 );

        fwrite( &vtx, sizeof vtx, 1, outFile );
      }
    }
    else
    {
      Vertex vtx;
      for ( unsigned vtxIx = 0; vtxIx < mesh->mNumVertices; vtxIx++ )
      {
        aiVector3D v = mesh->mVertices          [ vtxIx ];
        aiVector3D n = mesh->mNormals           [ vtxIx ];
        aiVector3D t = mesh->mTangents          [ vtxIx ];
        aiVector3D b = mesh->mBitangents        [ vtxIx ];
        aiVector3D x = mesh->mTextureCoords[ 0 ][ vtxIx ];

        v *= xform;
        n *= xform;
        t *= xform;
        b *= xform;

        n.Normalize();
        t.Normalize();
        b.Normalize();

        Compress( vtx.position,  v, scale );
        Compress( vtx.normal,    n );
        Compress( vtx.tangent,   t );
        Compress( vtx.bitangent, b );

        Float16::Convert( &vtx.texcoord[ 0 ], x.x );
        Float16::Convert( &vtx.texcoord[ 1 ], x.y );

        fwrite( &vtx, sizeof vtx, 1, outFile );
      }
    }
  }

  int meshReindex = 0;
  index_t vertexBase = 0;
  for ( unsigned meshIx = 0; meshIx < scene->mNumMeshes; meshIx++ )
  {
    aiMesh* mesh = scene->mMeshes[ meshIx ];

    if ( !Validate( mesh, meshIx ) )
      continue;

    if ( IsLowerLOD( mesh ) )
      continue;

    for ( unsigned faceIx = 0; faceIx < mesh->mNumFaces; faceIx++ )
    {
      const aiFace& face = mesh->mFaces[ faceIx ];

      index_t indices[ 3 ] = { 0, 0, 0 };
      if ( face.mNumIndices != 3 )
        printf_s( "Face has %u indices in submesh %d.\n", face.mNumIndices, meshIx );

      for ( unsigned ix = 0; ix < std::min( face.mNumIndices, 3U ); ix++ )
      {
        auto index = uint32_t( face.mIndices[ ix ] ) + uint32_t( vertexBase );
        if ( sizeof index_t == 2 && index > 0x0000FFFFU )
        {
          static bool once = true;
          if ( once )
          {
            once = false;
            printf_s( "Conversion should be 32 bit index.\n" );
          }
        }

        indices[ ix ] = index_t( index );
      }

      fwrite( &indices, sizeof indices, 1, outFile );
    }

    vertexBase += vertexCounts[ meshReindex ];
    meshReindex++;
  }

  fclose( outFile );

  std::map< unsigned, GUID > materialGUIDMap;

  auto rootSceneNode = scene->mRootNode;

  while ( rootSceneNode->mNumChildren == 1 && rootSceneNode->mChildren[ 0 ]->mNumMeshes == 0 )
    rootSceneNode = rootSceneNode->mChildren[ 0 ];

  for ( unsigned materialIx = 0; materialIx < scene->mNumMaterials; materialIx++ )
  {
    auto material = scene->mMaterials[ materialIx ];

    aiString materialName;
    aiString diffuseMap, normalMap, roughnessMap, metallicMap, emissiveMap;
    float    metallicValue  = -1;
    float    roughnessValue = -1;
    bool     scaleuv        = false;
    bool     onebitalpha    = false;
    bool     alphablend     = false;
    bool     twosided       = false;
    bool     wetness        = false;

    materialName = material->GetName();

    auto mnode = (XMLElement*)mxml->InsertEndChild( mxml->GetDocument()->NewElement( "Material" ) );
    mnode->SetAttribute( "basePath", basePath );

    GUID materialGUID;
    CoCreateGuid( &materialGUID );
    materialGUIDMap[ materialIx ] = materialGUID;

    for ( unsigned propIx = 0; propIx < material->mNumProperties; ++propIx )
    {
      if ( strcmp( material->mProperties[ propIx ]->mKey.C_Str(), "$raw.scaleuv" ) == 0 )
        scaleuv = true;
      if ( strcmp( material->mProperties[ propIx ]->mKey.C_Str(), "$raw.onebitalpha" ) == 0 )
        onebitalpha = true;
      if ( strcmp( material->mProperties[ propIx ]->mKey.C_Str(), "$raw.alphablend" ) == 0 )
        alphablend = true;
      if ( strcmp( material->mProperties[ propIx ]->mKey.C_Str(), "$raw.twosided" ) == 0 )
        twosided = true;
      if ( strcmp( material->mProperties[ propIx ]->mKey.C_Str(), "$raw.use_wetness" ) == 0 )
        wetness = true;
      if ( strcmp( material->mProperties[ propIx ]->mKey.C_Str(), "$raw.metallic_value" ) == 0 )
        material->Get( "$raw.metallic_value", 0, 0, metallicValue );
      if ( strcmp( material->mProperties[ propIx ]->mKey.C_Str(), "$raw.roughness_value" ) == 0 )
        material->Get( "$raw.roughness_value", 0, 0, roughnessValue );
    }

    if ( material->GetTextureCount( aiTextureType_NORMALS ) > 0 )
    {
      material->GetTexture( aiTextureType_NORMALS, 0, &normalMap );

      if ( !noTexture )
        ConvertTexture( normalMap.C_Str(), dstFile, "ATI2N" );
    }

    // Blender exports roughness map in shininess
    if ( material->GetTextureCount( aiTextureType_SHININESS ) > 0 )
    {
      material->GetTexture( aiTextureType_SHININESS, 0, &roughnessMap );

      if ( !noTexture )
        ConvertTexture( roughnessMap.C_Str(), dstFile, "ATI1N" );
    }

    if ( material->GetTextureCount( aiTextureType_METALNESS ) > 0 )
    {
      material->GetTexture( aiTextureType_METALNESS, 0, &metallicMap );

      if ( !noTexture )
        ConvertTexture( metallicMap.C_Str(), dstFile, "ATI1N" );
    }

    if ( material->GetTextureCount( aiTextureType_EMISSIVE ) > 0 )
    {
      material->GetTexture( aiTextureType_EMISSIVE, 0, &emissiveMap );

      if ( !noTexture )
        ConvertTexture( emissiveMap.C_Str(), dstFile, "DXT" );
    }

    if ( material->GetTextureCount( aiTextureType_DIFFUSE ) > 0 )
    {
      material->GetTexture( aiTextureType_DIFFUSE, 0, &diffuseMap );

      if ( !noTexture )
        ConvertTexture( diffuseMap.C_Str(), dstFile, "DXT" );

      size_t uix = diffuseMap.length - 5;
      if ( tolower( diffuseMap.C_Str()[ uix ] ) == 'a' )
      {
        if ( normalMap.length == 0 )
        {
          normalMap = diffuseMap;
          normalMap.data[ uix ] = 'N';
          if ( GetFileAttributesA( normalMap.C_Str() ) == INVALID_FILE_ATTRIBUTES )
            normalMap.Clear();
          else if ( !noTexture )
            ConvertTexture( normalMap.C_Str(), dstFile, "ATI2N" );
        }

        if ( metallicMap.length == 0 )
        {
          metallicMap = diffuseMap;
          metallicMap.data[ uix ] = 'M';
          if ( GetFileAttributesA( metallicMap.C_Str() ) == INVALID_FILE_ATTRIBUTES )
            metallicMap.Clear();
          else if ( !noTexture )
            ConvertTexture( metallicMap.C_Str(), dstFile, "ATI1N" );
        }

        if ( roughnessMap.length == 0 )
        {
          roughnessMap = diffuseMap;
          roughnessMap.data[ uix ] = 'R';
          if ( GetFileAttributesA( roughnessMap.C_Str() ) == INVALID_FILE_ATTRIBUTES )
            roughnessMap.Clear();
          else if ( !noTexture )
            ConvertTexture( roughnessMap.C_Str(), dstFile, "ATI1N" );
        }
      }
    }
    else
      diffuseMap.Clear();

    printf_s( "Material: %s, index %u, diffuse: %s\n", material->GetName().C_Str(), materialIx, diffuseMap.C_Str() );

    mnode->SetAttribute( "name", materialName.C_Str() );
    mnode->SetAttribute( "guid", to_string( materialGUID ).data() );

    struct local
    {
      static void ToDDS( aiString& s )
      {
        s.data[ s.length - 3 ] = 'd';
        s.data[ s.length - 2 ] = 'd';
        s.data[ s.length - 1 ] = 's';
      }
    };

    if ( diffuseMap.length > 0 )
    {
      local::ToDDS( diffuseMap );
      auto dnode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "ColorChannel" ) );
      dnode->SetAttribute( "path", PathFindFileNameA( diffuseMap.C_Str() ) );
    }
    if ( normalMap.length > 0 )
    {
      local::ToDDS( normalMap );
      auto nnode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "NormalChannel" ) );
      nnode->SetAttribute( "path", PathFindFileNameA( normalMap.C_Str() ) );
    }
    if ( metallicMap.length > 0 )
    {
      local::ToDDS( metallicMap );
      auto snode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "MetallicChannel" ) );
      snode->SetAttribute( "path", PathFindFileNameA( metallicMap.C_Str() ) );
    }
    if ( roughnessMap.length > 0 )
    {
      local::ToDDS( roughnessMap );
      auto rnode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "RoughnessChannel" ) );
      rnode->SetAttribute( "path", PathFindFileNameA( roughnessMap.C_Str() ) );
    }
    if ( emissiveMap.length > 0 )
    {
      local::ToDDS( emissiveMap );
      auto enode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "EmissiveChannel" ) );
      enode->SetAttribute( "path", PathFindFileNameA( emissiveMap.C_Str() ) );
    }
    if ( scaleuv )
    {
      auto snode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "Sampler" ) );
      snode->SetAttribute( "scaleuv", "yes" );
    }
    if ( onebitalpha )
    {
      auto bnode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "BlendMode" ) );
      bnode->SetAttribute( "type", "AlphaTest" );
    }
    if ( alphablend )
    {
      auto bnode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "BlendMode" ) );
      bnode->SetAttribute( "type", "Blend" );
    }
    if ( twosided )
    {
      auto rnode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "Raster" ) );
      rnode->SetAttribute( "twosided", "yes" );
    }
    if ( wetness )
    {
      auto snode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "Shader" ) );
      snode->SetAttribute( "wetness", "yes" );
    }
    if ( metallicMap.length == 0 && metallicValue >= 0 )
    {
      auto snode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "MetallicChannel" ) );
      snode->SetAttribute( "value", metallicValue );
    }
    if ( roughnessMap.length == 0 && roughnessValue >= 0 )
    {
      auto snode = (XMLElement*)mnode->InsertEndChild( mxml->GetDocument()->NewElement( "RoughnessChannel" ) );
      snode->SetAttribute( "value", roughnessValue );
    }
  }

  for ( unsigned entityIx = 0; entityIx < rootSceneNode->mNumChildren; entityIx++ )
  {
    auto entityNode = rootSceneNode->mChildren[ entityIx ];
    if ( entityNode->mNumMeshes == 0 )
      continue;

    if ( IsLowerLOD( entityNode ) )
      continue;

    GUID entityGUID;
    CoCreateGuid( &entityGUID );

    auto enode = (XMLElement*)exml->InsertEndChild( exml->GetDocument()->NewElement( "Entity" ) );
    enode->SetAttribute( "name", NormalizeEntityName( entityNode ).data() );
    enode->SetAttribute( "category", "fillme" );
    enode->SetAttribute( "guid", to_string( entityGUID ).data() );

    auto vnode = (XMLElement*)enode->InsertEndChild( exml->GetDocument()->NewElement( "Visuals" ) );
    auto fnode = (XMLElement*)vnode->InsertEndChild( exml->GetDocument()->NewElement( "Frame" ) );
    vnode->SetAttribute( "basePath", basePath );
    fnode->SetAttribute( "name", "default" );

    std::set< unsigned > usedMats;
    for ( unsigned meshIx = 0; meshIx < entityNode->mNumMeshes; meshIx++ )
    {
      auto mesh    = scene->mMeshes[ entityNode->mMeshes[ meshIx ] ];
      auto matIx   = mesh->mMaterialIndex;
      auto matGUID = materialGUIDMap[ matIx ];

      if ( usedMats.find( matIx ) != usedMats.end() )
        continue;

      usedMats.insert( matIx );

      auto mnode = (XMLElement*)vnode->InsertEndChild( exml->GetDocument()->NewElement( "MaterialRef" ) );
      mnode->SetAttribute( "id", matIx );
      mnode->SetAttribute( "guid", to_string( matGUID ).data() );
    }

    for ( unsigned meshIx = 0; meshIx < entityNode->mNumMeshes; meshIx++ )
    {
      auto meshIndex = entityNode->mMeshes[ meshIx ];
      auto mesh      = scene->mMeshes[ meshIndex ];

      if ( !Validate( mesh, meshIndex ) )
        continue;

      if ( IsLowerLOD( mesh ) )
        continue;

      // meshIndex should be decreased by the number of meshes discarded before meshIndex
      meshIndex = meshRemap.find( meshIndex )->second;

      auto mnode = (XMLElement*)fnode->InsertEndChild( exml->GetDocument()->NewElement( "Mesh" ) );
      mnode->SetAttribute( "path", PathFindFileNameA( dstFile ) );
      mnode->SetAttribute( "submesh", meshIndex );
    }
  }

  if ( scene->HasAnimations() )
  {
    char animPath[ 4096 ];
    sprintf_s( animPath, "%s.anim", dstFile );

    FILE* animFile = nullptr;
    if ( fopen_s( &animFile, animPath, "wb" ) )
    {
      printf_s( "Failed to open %s for writing.\n", animPath );
      return false;
    }

    Write( animFile, ModelFeatures::animFCC );

    uint32_t features = 0;
    Write( animFile, features );

    Write( animFile, uint32_t( scene->mNumAnimations ) );
    for ( unsigned animIx = 0; animIx < scene->mNumAnimations; animIx++ )
    {
      const aiAnimation* animation = scene->mAnimations[ animIx ];

      fwrite( animation->mName.C_Str(), 1, animation->mName.length + 1, animFile );
      Write( animFile, uint32_t( animation->mNumChannels ) );
      for ( unsigned channelIx = 0; channelIx < animation->mNumChannels; channelIx++ )
      {
        const aiNodeAnim* channel = animation->mChannels[ channelIx ];

        fwrite( channel->mNodeName.C_Str(), 1, channel->mNodeName.length + 1, animFile );

        Write( animFile, uint32_t( channel->mNumScalingKeys ) );
        for ( unsigned keyIx = 0; keyIx < channel->mNumScalingKeys; keyIx++ )
        {
          const aiVectorKey& key = channel->mScalingKeys[ keyIx ];
          Write( animFile, uint32_t( round( key.mTime ) ) );
          Write( animFile, float( key.mValue.x ) );
          Write( animFile, float( key.mValue.y ) );
          Write( animFile, float( key.mValue.z ) );
        }
        Write( animFile, uint32_t( channel->mNumRotationKeys ) );
        for ( unsigned keyIx = 0; keyIx < channel->mNumRotationKeys; keyIx++ )
        {
          const aiQuatKey& key = channel->mRotationKeys[ keyIx ];
          Write( animFile, uint32_t( round( key.mTime ) ) );
          Write( animFile, float( key.mValue.x ) );
          Write( animFile, float( key.mValue.y ) );
          Write( animFile, float( key.mValue.z ) );
          Write( animFile, float( key.mValue.w ) );
        }
        Write( animFile, uint32_t( channel->mNumPositionKeys ) );
        for ( unsigned keyIx = 0; keyIx < channel->mNumPositionKeys; keyIx++ )
        {
          const aiVectorKey& key = channel->mPositionKeys[ keyIx ];
          Write( animFile, uint32_t( round( key.mTime ) ) );
          Write( animFile, float( key.mValue.x ) * scale );
          Write( animFile, float( key.mValue.y ) * scale );
          Write( animFile, float( key.mValue.z ) * scale );
        }
      }
    }

    tinyxml2::XMLDocument xmlDoc;
    auto rootNode = xmlDoc.InsertFirstChild( xmlDoc.NewElement( "Frames" ) );
    ExportFrame( scene->mRootNode, rootNode, boneNames );
    xmlDoc.SaveFile( animFile, true );

    fclose( animFile );
  }

  return true;
}

void FindJobs( std::vector< std::pair< std::string, std::string > >& jobs, const char* srcFolder, const char* srcType, const char* dstFolder )
{
  char searchPath[ MAX_PATH ];
  PathCombineA( searchPath, srcFolder, srcType );

  WIN32_FIND_DATAA findData;
  auto finder = FindFirstFileA( searchPath, &findData );
  if ( finder == INVALID_HANDLE_VALUE )
    return;

  do
  {
    char s[ MAX_PATH ];
    char d[ MAX_PATH ];

    PathCombineA( s, srcFolder, findData.cFileName );
    PathCombineA( d, dstFolder, findData.cFileName );
    strcat_s( d, ".mesh" );

    jobs.emplace_back( s, d );
  } while ( FindNextFileA( finder, &findData ) );

  FindClose( finder );
}

int main( size_t argc, char** argv )
{
  if ( argc < 4 )
  {
    printf_s( "ModelConverter.exe scale src_path dst_path base_path\n" );
    return -1;
  }

  float       scale    = float( atof( argv[ 1 ] ) );
  const char* srcFile  = argv[ 2 ];
  const char* dstFile  = argv[ 3 ];
  const char* basePath = argv[ 4 ];

  bool index32   = false;
  bool noTexture = false;
  bool rotate90  = false;
  if ( argc > 5 )
  {
    for ( size_t aix = 5; aix < argc; aix++ )
    {
      if ( _stricmp( argv[ aix ], "--32bit" ) == 0 )
        index32 = true;
      else if ( _stricmp( argv[ aix ], "--notexture" ) == 0 )
        noTexture = true;
      else if ( _stricmp( argv[ aix ], "--rotate90" ) == 0 )
        rotate90 = true;
    }
  }

  auto srcFileAttribs = GetFileAttributesA( srcFile );
  auto dstFileAttribs = GetFileAttributesA( dstFile );
  bool isBatch     = ( srcFileAttribs & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY;
  bool dstIsFolder = ( dstFileAttribs & FILE_ATTRIBUTE_DIRECTORY ) == FILE_ATTRIBUTE_DIRECTORY;
  if ( isBatch && ( dstFileAttribs == INVALID_FILE_ATTRIBUTES || !dstIsFolder ) )
  {
    printf_s( "For batch conversion dst_path should be a folder.\n" );
    return -1;
  }

  std::vector< std::pair< std::string, std::string > > jobs;
  if ( isBatch )
  {
    FindJobs( jobs, srcFile, "*.obj", dstFile );
    FindJobs( jobs, srcFile, "*.fbx", dstFile );
    FindJobs( jobs, srcFile, "*.x",   dstFile );
  }
  else
  {
    jobs.emplace_back( srcFile, dstFile );
  }

  tinyxml2::XMLDocument xmlDoc;
  auto rootNode = xmlDoc.InsertFirstChild( xmlDoc.NewElement( "UWPSandboxGame" ) );
  auto entitiesNode = rootNode->InsertFirstChild( xmlDoc.NewElement( "Entities" ) );
  auto materialsNode = rootNode->InsertFirstChild( xmlDoc.NewElement( "EntityMaterials" ) );

  for ( const auto& job : jobs )
  {
    if ( index32 )
    {
      if ( !ConvertFile< uint32_t >( scale, job.first.data(), job.second.data(), entitiesNode, materialsNode, basePath, noTexture, rotate90 ) )
        printf_s( "Failed to convert %s.\n", job.first.data() );
    }
    else
    {
      if ( !ConvertFile< uint16_t >( scale, job.first.data(), job.second.data(), entitiesNode, materialsNode, basePath, noTexture, rotate90 ) )
        printf_s( "Failed to convert %s.\n", job.first.data() );
    }
  }

  char entityDescPath[ MAX_PATH ];
  if ( dstIsFolder )
  {
    PathCombineA( entityDescPath, dstFile, "game.xml" );
  }
  else
  {
    strcpy_s( entityDescPath, dstFile );
    strcat_s( entityDescPath, ".xml" );
  }

  xmlDoc.SaveFile( entityDescPath, false );
}
