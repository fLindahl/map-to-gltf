#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <filesystem>
#include "exts/flags.h"
#include "exts/fx/gltf.h"
#include "map.h"
#include "mapconverter.h"

//------------------------------------------------------------------------------
/**
*/
void
PrintHelp()
{
    std::cout <<
        "Usage: mtg [path to input file] [args]\n"
        "Available args:\n"
        "--help, -h \t Print cli argument information.\n"
        "-glb \t File should be exported into GLB format.\n"
        "-o [file]\t Path to output file. If not specified, the file will be placed adjacent to the input file but with different extension.\n"
        "-unify\t Perform CSG union between all brushes of an entity. This reduces the amount of output meshes and polygons.\n"
        "-copyright [copyright notice]\t Specify a copyright notice that will be embeded in the exported file.\n"
        "-filter\t Use linear filtering for all textures.\n"
        "-scale [float]\t Bake given scale into meshes, Default is 1.0, which makes 64 MAP units to correspond to 1.0f GLTF units (meters).\n"
        "-lh\t Export using left-handed coordinate system instead of GLTFs default right-handed system.\n"
        "-embed\t Embed textures in the output.\n"
        "-physics\t export OMI physics collider nodes\n"
        "-texroot [folder name]\t Specify a texture root folder relative to cwd (default: \"textures\").\n"
        "\t\t\t Note that your cwd needs to be the same as the output directory.\n"
        "\t\t\t for the gltf to be able to find the correct path.\n\n"
        << std::endl;
}

//------------------------------------------------------------------------------
/**
*/
void print_what(const std::exception& e);

//------------------------------------------------------------------------------
/**
*/
int
main(int argc, char** argv)
{
    const flags::args args(argc, argv);
    if (args.get<bool>("help", false) || args.get<bool>("h", false))
    {
        PrintHelp();
        return 0;
    }

    const auto& allArgs = args.positional();
    if (allArgs.size() < 1)
    {
        std::cerr << "No input file specified!" << std::endl;
        return 1;
    }
    
    bool produceGlb         = args.get("glb", false);
    bool generatePhysics    = args.get<bool>("physics", false);
    bool filter             = args.get<bool>("filter", false);
    bool embedImages        = args.get<bool>("filter", false);
    float meshScale         = args.get<float>("scale", 1.0f);
    bool useLH              = args.get<bool>("lh", false);

    std::filesystem::path inputFilePath = allArgs.front();
    std::filesystem::path outputFilePath = args.get<std::string>("o", inputFilePath.string());
    if (produceGlb)
        outputFilePath.replace_extension(".glb");
    else
        outputFilePath.replace_extension(".gltf");

    std::vector<Entity> entities;
    std::vector<Texture> textures;

    MAPFile mapFile;
    mapFile.meshScale   = meshScale;
    mapFile.useLH       = useLH;
    mapFile.unify       = args.get<bool>("unify", false);
    mapFile.textureRoot = args.get<std::string>("texroot", "textures");
    mapFile.physics     = generatePhysics;
    
    if (mapFile.Load(inputFilePath.string().c_str(), entities, textures))
    {
        using namespace fx;

        gltf::Document doc;
        doc.asset.generator = "map-to-gltf by Fredrik Lindahl";
        doc.asset.copyright = args.get<std::string>("copyright", {});

        { // setup default scene
            gltf::Scene scene;
            scene.name = "";
            doc.scenes.push_back(std::move(scene));
            doc.scene = 0;
        }

        MapConverter::CreateNodes(doc, entities);
        MapConverter::CreateMeshes(doc, entities, produceGlb, outputFilePath);
        MapConverter::SetupProperties(doc, entities, meshScale, useLH);
        MapConverter::CreateTextures(doc, textures, filter, embedImages, mapFile.textureRoot + "/");

        if (generatePhysics)
        {
            MapConverter::GeneratePhysicsNodes(doc, entities);
        }

        // Save document

        try
        {
            gltf::Save(doc, outputFilePath, produceGlb);
        }
        catch (const std::exception& e)
        {
            print_what(e);
            return 1;
        }

        return 0;
    }

    return 1;
}

//------------------------------------------------------------------------------
/**
*/
void
print_what(const std::exception& e)
{
    std::cerr << e.what() << '\n';
    try {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& nested) {
        std::cerr << "nested: ";
        print_what(nested);
    }
}