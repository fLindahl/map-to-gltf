#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <filesystem>
#include "flags.h"
#include "exts/fx/gltf.h"
#include "exts/map-files/map.h"

void PrintHelp()
{
    std::cout <<
        "Usage: mtg [path to input file] [args]\n"
        "Available args:"
        "--help, -h \t Print cli argument information.\n"
        "-glb \t File should be exported into GLB format.\n"
        "-o [file]\t Path to output file. If not specified, the file will be placed adjacent to the input file but with different extension.\n"
        "-unify\t Perform CSG union between all brushes of an entity. This reduces the amount of ouput meshes and polygons.\n"
        "-texroot [folder name]\t Specify a texture root folder relative to cwd (default: \"textures\")\n"
        "-copyright [copyright notice]\t Specify a copyright notice that will be embeded in the exported file.\n"
        << std::endl;
}

int main(int argc, char** argv)
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
    
    std::filesystem::path inputFilePath = allArgs.front();
    std::filesystem::path outputFilePath;
    
    auto optOutputPath = args.get<std::string>("o");
    if (optOutputPath.has_value())
        outputFilePath = optOutputPath.value();
    else
    {
        outputFilePath = inputFilePath;
    
        if (args.get("glb", false))
            outputFilePath.replace_extension(".glb");
        else
            outputFilePath.replace_extension(".gltf");
    }
    
    std::cout << "Converting: " << inputFilePath << " to "
        << outputFilePath << "..." << std::endl;
    
    MAPFile mapFile;
    mapFile.unify = args.get<bool>("unify", false);
    mapFile.textureRoot = args.get<std::string>("texRoot", "textures");

    std::vector<MapEntity> entities;
    std::vector<MapTexture> textures;
    std::string inputFilePathString = inputFilePath.string();
    bool loaded = mapFile.Load(inputFilePathString.c_str(), entities, textures);
    if (loaded)
    {
        using namespace fx;

        gltf::Document doc;
        doc.asset.generator = "map-to-gltf by Fredrik Lindahl";
        doc.asset.copyright = args.get<std::string>("copyright", "");

        {
            gltf::Scene scene;
            scene.name = "";
            scene.nodes.push_back(0);
            doc.scenes.push_back(std::move(scene));
            doc.scene = 0;
        }

        for (auto const& entity : entities)
        {
            gltf::Node node;
            entity.properties
        }

        return 0;
    }

    return 1;
}
