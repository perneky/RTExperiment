#pragma once

#include "Render/Resource.h"
#include "Render/Device.h"
#include "Render/CommandList.h"
#include "Render/Types.h"

template< typename T >
inline std::unique_ptr< Resource > CreateBufferFromData( const T* firstElement
                                                       , int elementCount
                                                       , ResourceType resourceType
                                                       , Device& device
                                                       , CommandList& commandList
                                                       , const wchar_t* debugName )
{
  int  es = sizeof( T );
  int  bs = elementCount * es;

  auto resultBuffer = device.CreateBuffer( resourceType, HeapType::Default, false, bs, es, debugName );
  auto uploadBuffer = RenderManager::GetInstance().GetUploadConstantBufferForResource( *resultBuffer );
  commandList.UploadBufferResource( std::move( uploadBuffer ), *resultBuffer, firstElement, es * elementCount );
  return resultBuffer;
}