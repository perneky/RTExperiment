#pragma once

struct ModelFeatures
{
  static const uint32_t modelFCC = 'B3DM';
  static const uint32_t animFCC = 'B3DA';

  enum Type
  {
    Skin = 1 << 0,
    Index32 = 1 << 1,
  };
};

