#include "MeasureCPUTime.h"
#include "Device.h"
#include "GPUTimeQuery.h"

MeasureCPUTime::MeasureCPUTime( Device& device )
{
  startQuery = device.CreateGPUTimeQuery();
  stopQuery  = device.CreateGPUTimeQuery();
}

MeasureCPUTime::~MeasureCPUTime()
{
}

void MeasureCPUTime::Start( CommandList& commandList )
{
  assert( !started );
  assert( !stopped );

  startQuery->Insert( commandList );
  started = true;
}

void MeasureCPUTime::Stop( CommandList& commandList )
{
  assert( started );
  assert( !stopped );

  stopQuery->Insert( commandList );
  stopped = true;
}

void MeasureCPUTime::Reset()
{
  started = stopped = false;
}

double MeasureCPUTime::GetResult( CommandList& commandList )
{
  assert( started && stopped );

  auto startResult = startQuery->GetResult( commandList );
  auto stopResult  = stopQuery ->GetResult( commandList );

  if ( startResult < 0 || stopResult < 0 )
    return -1;

  return stopResult - startResult;
}

MeasureCPUTime::Status MeasureCPUTime::GetStatus() const
{
  if ( !started && !stopped )
    return Status::Idle;

  if ( started && !stopped )
    return Status::Started;

  assert( started && stopped );
  return Status::Stopped;
}
