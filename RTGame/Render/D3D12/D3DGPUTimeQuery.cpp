#include "D3DGPUTimeQuery.h"
#include "D3DDevice.h"
#include "D3DCommandList.h"
#include "D3DResource.h"

D3DGPUTimeQuery::D3DGPUTimeQuery( D3DDevice& device )
{
  D3D12_QUERY_HEAP_DESC desc = {};
  desc.Type     = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
  desc.Count    = 1;
  desc.NodeMask = 0;
  device.GetD3DDevice()->CreateQueryHeap( &desc, IID_PPV_ARGS( &d3dQueryHeap ) );
  d3dQueryHeap->SetName( L"Timestamp query heap" );

  resultBuffer.reset( static_cast< D3DResource* >( device.CreateBuffer( ResourceType::ConstantBuffer, HeapType::Readback, false, sizeof( uint64_t ), sizeof( uint64_t ), L"TimestampQueryBuffer" ).release() ) );
}

D3DGPUTimeQuery::~D3DGPUTimeQuery()
{
}

double D3DGPUTimeQuery::GetResult( CommandList& commandList )
{
  if ( result >= 0 )
    return result;

  if ( !gotResult )
    return -1;

  auto d3dCommandList = static_cast< D3DCommandList* >( &commandList );
  d3dCommandList->GetD3DGraphicsCommandList()->ResolveQueryData( d3dQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, 1, resultBuffer->GetD3DResource(), 0 );

  if ( auto data = resultBuffer->Map() )
  {
    uint64_t ticks;
    memcpy_s( &ticks, sizeof( ticks ), data, sizeof( uint64_t ) );
    resultBuffer->Unmap();

    result = ticks / double( d3dCommandList->GetFrequency() );
  }

  return result;
}

void D3DGPUTimeQuery::Insert( CommandList& commandList )
{
  result = -1;
  gotResult = false;

  auto d3dCommandList = static_cast< D3DCommandList* >( &commandList )->GetD3DGraphicsCommandList();
  d3dCommandList->EndQuery( d3dQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0 );

  commandList.RegisterEndFrameCallback( [this]() 
  {
    gotResult = true;
  } );
}
