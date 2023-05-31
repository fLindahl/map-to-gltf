#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <filesystem>
#include "flags.h"
#include "exts/fx/gltf.h"
#include "exts/map-files/map.h"
#include <assert.h>

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
        "-filter\t Use linear filtering for all textures.\n"
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
        doc.asset.copyright = args.get<std::string>("copyright", {});

        { // setup scenes
            gltf::Scene scene;
            scene.name = "";
            scene.nodes.push_back(0);
            doc.scenes.push_back(std::move(scene));
            doc.scene = 0;
        }

        int32_t samplerDefaultId = 0;
        { // setup samplers
            gltf::Sampler samplerDefault;
            bool filter = args.get<bool>("filter", false);
            samplerDefault.magFilter = filter ? gltf::Sampler::MagFilter::Linear : gltf::Sampler::MagFilter::Nearest;
            samplerDefault.minFilter = filter ? gltf::Sampler::MinFilter::LinearMipMapLinear : gltf::Sampler::MinFilter::Nearest;
            samplerDefaultId = doc.samplers.size();
            doc.samplers.push_back(std::move(samplerDefault));
        }

        for (auto const& texture : textures)
        {
            gltf::Material mat;
            gltf::Texture tex;
            
            gltf::Image img;
            img.uri = mapFile.textureRoot + "/" + texture.name;
            int32_t const imgId = doc.images.size();

#ifdef _DEBUG
            int32_t const texId = doc.textures.size();
            int32_t const matId = doc.materials.size();
            // Textures and materials in the gltf should be adjacent to the map textures
            assert(imgId == texture.id && texId == texture.id && matId == texture.id);
#endif

            tex.source = imgId;
            tex.name = std::filesystem::path(texture.name).replace_extension("").string();
            tex.sampler = samplerDefaultId;
            
            doc.images.push_back(std::move(img));
            doc.textures.push_back(std::move(tex));
            doc.materials.push_back(std::move(mat));
        }

        gltf::Buffer vertexBuffer;

        size_t vertexBufferSize = 0;
        for (auto const& entity : entities)
        {
            for (size_t i = 0; i < entity.primitives.size(); i++)
            {
                vertexBufferSize += entity.primitives[i].vertexBuffer.size() * sizeof(Primitive::VertexPNT);
            }
        }
        
        vertexBuffer.data.resize(vertexBufferSize);
        size_t byteOffset = 0;
        uint8_t* bufOffset = vertexBuffer.data.data();
        
        uint32_t const vertexBufferIndex = doc.buffers.size();
        doc.buffers.push_back(std::move(vertexBuffer));

        for (auto const& entity : entities)
        {
            gltf::Node node;
            auto nameIt = entity.properties.find("_tb_name");
            if (nameIt != entity.properties.end())
            {
                node.name = nameIt->second;
            }
            
            gltf::Mesh mesh;

            for (size_t i = 0; i < entity.primitives.size(); i++)
            {
                Primitive const& primitive = entity.primitives[i];
                size_t numBytes = primitive.vertexBuffer.size() * sizeof(Primitive::VertexPNT);
                std::memcpy(bufOffset, primitive.vertexBuffer.data(), numBytes);
                
                gltf::BufferView view;
                view.buffer = vertexBufferIndex;
                view.byteOffset = byteOffset;
                view.byteLength = numBytes;
                view.target = gltf::BufferView::TargetType::ArrayBuffer;
                uint32_t const viewIndex = doc.bufferViews.size();
                doc.bufferViews.push_back(view);

                gltf::Accessor posAccessor;
                posAccessor.min = { (float)primitive.min.x, (float)primitive.min.y, (float)primitive.min.z };
                posAccessor.max = { (float)primitive.max.x, (float)primitive.max.y, (float)primitive.max.z };
                posAccessor.bufferView = viewIndex;
                posAccessor.count = primitive.vertexBuffer.size();
                posAccessor.type = gltf::Accessor::Type::Vec3;
                posAccessor.componentType = gltf::Accessor::ComponentType::Float;
                posAccessor.


                gltf::Primitive gltfPrimitive;
                gltfPrimitive.mode = gltf::Primitive::Mode::Triangles;
                gltfPrimitive.material = primitive.textureId;

                gltfPrimitive.attributes = {
                    {"POSITION", 1},
                    {"NORMAL", 2},
                    {"TEXCOORD_0", 3}
                };

                bufOffset += numBytes;
                byteOffset += numBytes;
            }
        }

        return 0;
    }

    return 1;
}
