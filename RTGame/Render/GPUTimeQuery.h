#pragma once

struct CommandList;

struct GPUTimeQuery
{
  virtual ~GPUTimeQuery() = default;

  virtual void   Insert( CommandList& commandList ) = 0;
  virtual double GetResult( CommandList& commandList ) = 0;
};