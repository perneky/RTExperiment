#pragma once

static void Icosahedron( std::vector< GIProbeVertexFormat >& vertices, std::vector< uint16_t >& indices )
{
  const float t = ( 1.0f + sqrtf( 5.0f ) ) / 2.0f;

  // Vertices
  { XMFLOAT3 s( -1.0f,     t,  0.0f ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(  1.0f,     t,  0.0f ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s( -1.0f,    -t,  0.0f ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(  1.0f,    -t,  0.0f ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(  0.0f, -1.0f,     t ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(  0.0f,  1.0f,     t ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(  0.0f, -1.0f,    -t ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(  0.0f,  1.0f,    -t ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(     t,  0.0f, -1.0f ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(     t,  0.0f,  1.0f ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(    -t,  0.0f, -1.0f ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }
  { XMFLOAT3 s(    -t,  0.0f,  1.0f ); XMHALF4 v; XMVECTOR x = XMVector3Normalize( XMLoadFloat3( &s ) ); XMStoreHalf4( &v, x ); vertices.emplace_back( GIProbeVertexFormat { v } ); }

  // Faces
  indices.insert( indices.end(), {  0, 11,  5 } );
  indices.insert( indices.end(), {  0,  5,  1 } );
  indices.insert( indices.end(), {  0,  1,  7 } );
  indices.insert( indices.end(), {  0,  7, 10 } );
  indices.insert( indices.end(), {  0, 10, 11 } );
  indices.insert( indices.end(), {  1,  5,  9 } );
  indices.insert( indices.end(), {  5, 11,  4 } );
  indices.insert( indices.end(), { 11, 10,  2 } );
  indices.insert( indices.end(), { 10,  7,  6 } );
  indices.insert( indices.end(), {  7,  1,  8 } );
  indices.insert( indices.end(), {  3,  9,  4 } );
  indices.insert( indices.end(), {  3,  4,  2 } );
  indices.insert( indices.end(), {  3,  2,  6 } );
  indices.insert( indices.end(), {  3,  6,  8 } );
  indices.insert( indices.end(), {  3,  8,  9 } );
  indices.insert( indices.end(), {  4,  9,  5 } );
  indices.insert( indices.end(), {  2,  4, 11 } );
  indices.insert( indices.end(), {  6,  2, 10 } );
  indices.insert( indices.end(), {  8,  6,  7 } );
  indices.insert( indices.end(), {  9,  8,  1 } );
}

struct Edge
{
  uint16_t v0;
  uint16_t v1;

  Edge( uint16_t v0, uint16_t v1 )
    : v0( v0 < v1 ? v0 : v1 )
    , v1( v0 < v1 ? v1 : v0 )
  {
  }

  bool operator < ( const Edge& rhs ) const
  {
    return v0 < rhs.v0 || ( v0 == rhs.v0 && v1 < rhs.v1 );
  }
};

static uint16_t subdivideEdge( uint16_t f0, uint16_t f1, FXMVECTOR v0, FXMVECTOR v1, std::vector< GIProbeVertexFormat >& vertices, std::map<Edge, uint16_t>& io_divisions )
{
  const Edge edge( f0, f1 );
  auto it = io_divisions.find( edge );
  if ( it != io_divisions.end() )
    return it->second;

  XMHALF4 h;
  XMStoreHalf4( &h, XMVector3Normalize( g_XMOneHalf * XMVectorAdd( v0, v1 ) ) );
  uint16_t f = uint16_t( vertices.size() );
  vertices.emplace_back( GIProbeVertexFormat { h } );
  io_divisions.emplace( edge, f );
  return f;
}

static void SubdivideMesh( std::vector< GIProbeVertexFormat >& vertices, std::vector< uint16_t >& indices )
{
  std::vector< GIProbeVertexFormat > verticesIn = std::move( vertices );
  std::vector< uint16_t >            indicesIn  = std::move( indices  );
  std::vector< GIProbeVertexFormat > verticesOut;
  std::vector< uint16_t >            indicesOut;

  verticesOut = verticesIn;

  std::map< Edge, uint16_t > divisions;

  for ( size_t i = 0; i < indicesIn.size() / 3; ++i )
  {
    const uint16_t f0 = indicesIn[ i * 3 + 0 ];
    const uint16_t f1 = indicesIn[ i * 3 + 1 ];
    const uint16_t f2 = indicesIn[ i * 3 + 2 ];

    const auto v0 = XMLoadHalf4( &verticesIn[ f0 ].position );
    const auto v1 = XMLoadHalf4( &verticesIn[ f1 ].position );
    const auto v2 = XMLoadHalf4( &verticesIn[ f2 ].position );

    const uint16_t f3 = subdivideEdge( f0, f1, v0, v1, verticesOut, divisions );
    const uint16_t f4 = subdivideEdge( f1, f2, v1, v2, verticesOut, divisions );
    const uint16_t f5 = subdivideEdge( f2, f0, v2, v0, verticesOut, divisions );

    indicesOut.insert( indicesOut.end(), {  f0, f3, f5 } );
    indicesOut.insert( indicesOut.end(), {  f3, f1, f4 } );
    indicesOut.insert( indicesOut.end(), {  f4, f2, f5 } );
    indicesOut.insert( indicesOut.end(), {  f3, f4, f5 } );
  }

  vertices.swap( verticesOut );
  indices.swap( indicesOut );
}