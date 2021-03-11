#pragma once

#include "Render/Types.h"

struct ModelSlot
{
  GUID                guid;
  std::wstring        path;
  int                 subset;
  std::vector< GUID > materials;
};

struct SlotEntry
{
  GUID model;
  GUID material;
};

using SlotConfig = std::map< GUID, SlotEntry >;

class GameDefinition
{
public:
  struct Sky
  {
    GUID         guid;
    std::wstring name;
    XMFLOAT3     lightColor;
    XMFLOAT3     lightDirection;
    float        lightIntensity;
    GUID         material;
  };

  using Skies = std::map< GUID, Sky >;

  struct Material
  {
    GUID         guid;
    std::wstring name;
    std::wstring thumbnailPath;
    std::wstring colorMapPath;
    std::wstring normalMapPath;
    std::wstring roughnessMapPath;
    std::wstring metallicMapPath;
    std::wstring emissiveMapPath;

    bool         clampToEdge = false;
    bool         flipNormalX = false;
    bool         scaleuv     = false;
    bool         twoSided    = false; // TODO
    bool         useWetness  = false;
    bool         useSpecular = false;

    float metallicValue  = 0;
    float roughnessValue = 0;

    BlendPreset blendMode = BlendPreset::DirectWrite;
  };

  struct Entity
  {
    struct Visuals
    {
      struct Frame
      {
        struct Content
        {
          Content& operator = ( const Content& other );

          enum class Type
          {
            Mesh,
            PointLight,
            Slot,
          };

          Type         type;
          std::wstring source;

          struct
          {
            struct
            {
              std::set< int > subsets;
            } mesh;
            struct
            {
              std::wstring                name;
              GUID                        guid;
              std::map< GUID, ModelSlot > models;
            } slot;
            struct
            {
              XMFLOAT3 color;
              float    minRange;
              float    maxRange;
              float    theta;
              float    phi;
              float    exponent;
              bool     isPoint;
              GUID     projector;
            } light;
          } data;
        };

        std::wstring           name;
        XMFLOAT4X4             transform;
        std::vector< Content > contents;
        std::vector< Frame   > children;
      };

      std::map< int, GUID > materials;
      std::vector< Frame >  frames;
      GUID                  animationSet;
    };

    GUID                        guid;
    std::wstring                name;
    std::wstring                thumbnail;
    std::vector< std::wstring > category;
    Visuals                     visuals;
  };

  struct Projector
  {
    struct Image
    {
      GUID         guid;
      std::wstring name;
      int          index;
    };

    bool         cube;
    std::wstring path;

    std::map< GUID, Image > images;
  };

  struct EntityBrush
  {
    std::wstring          name;
    GUID                  guid;
    std::map< GUID, int > entities;

    GUID GetRandomEntity() const;
  };

  using Materials     = std::map< GUID, Material    >;
  using Entities      = std::map< GUID, Entity      >;
  using EntityBrushes = std::map< GUID, EntityBrush >;

  struct EntityCategory
  {
    EntityCategory() = default;
    EntityCategory( const GUID& guid, const std::wstring& thumbnail )
      : guid( guid ), thumbnail( thumbnail ) {}

    GUID         guid;
    std::wstring thumbnail;
  };

  struct AnimationSet
  {
    struct Animation
    {
      std::wstring name;
      std::wstring group;

      int start;
      int end;
    };

    std::wstring name;
    GUID         guid;
    std::wstring path;

    std::vector< Animation > animations;
  };

  using AnimationSets = std::map< GUID, AnimationSet >;

  GameDefinition();

  const Materials&     GetTerrainMaterials() const { return terrainMaterials; }
  const Materials&     GetEntityMaterials () const { return entityMaterials;  }
  const Materials&     GetSkyMaterials    () const { return skyMaterials;     }
  const Entities &     GetEntities        () const { return entities;         }
  const Projector&     GetCubeProjectors  () const { return cubeProjectors;   }
  const EntityBrushes& GetEntityBrushes   () const { return entityBrushes;    }
  const AnimationSets& GetAnimationSets   () const { return animationSets;    }
  const Skies&         GetSkies           () const { return skies;            }

  std::vector< std::wstring >   GetCategoryFolders ( const std::vector< std::wstring >& path ) const;
  std::vector< EntityCategory > GetCategoryEntities( const std::vector< std::wstring >& path ) const;

  const std::wstring& GetEntityName( const GUID& guid ) const;

  bool   IsSlotEntity( const GUID& guid ) const;
  Entity GenerateEntityFromSlots( const GUID& guid, SlotConfig& slotConfig ) const;

  void Load( const DataChunk& data );

private:
  Materials     terrainMaterials;
  Materials     entityMaterials;
  Materials     skyMaterials;
  Entities      entities;
  Projector     cubeProjectors;
  Projector     twodProjectors;
  EntityBrushes entityBrushes;
  AnimationSets animationSets;
  Skies         skies;

  struct EntityHierarchy
  {
    std::map< GUID, std::wstring >            entities;
    std::map< std::wstring, EntityHierarchy > children;
  };

  EntityHierarchy entityHierarchy;
};

__declspec( selectany ) GameDefinition gameDefinition;