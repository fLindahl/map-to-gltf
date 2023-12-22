#pragma once
#include "exts/fx/gltf.h"
#include "entity.h"

class MapConverter
{
public:
    static void CreateNodes(fx::gltf::Document& doc, std::vector<Entity> const& entities);

    static void CreateMeshes(
            fx::gltf::Document& doc,
            std::vector<Entity> const& entities,
            bool produceGlb,
            std::filesystem::path const& outputFilePath
        );

    static void SetupProperties(fx::gltf::Document& doc, std::vector<Entity> const& entities, float meshScale, bool useLH);

    static void GeneratePhysicsNodes(fx::gltf::Document& doc, std::vector<Entity> const& entities);

    static void CreateTextures(fx::gltf::Document& doc, std::vector<Texture> const& textures, bool filter, bool embedImages, std::string const& textureRoot);
};
