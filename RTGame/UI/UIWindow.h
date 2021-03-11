#pragma once

struct CommandList;

struct UIWindow
{
  virtual ~UIWindow() = default;

  virtual void Tick( CommandList& commandList, double timeElapsed ) = 0;
};