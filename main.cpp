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
    Entity* entities = nullptr;
    Texture* textures = nullptr;
    std::string inputFilePathString = inputFilePath.string();
    mapFile.Load(inputFilePathString.c_str(), entities, textures);

    return 0;
}
