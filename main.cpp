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
        "Available args:\n"
        "--help, -h \t Print cli argument information.\n"
        "-glb \t File should be exported into GLB format.\n"
        "-o [file]\t Path to output file. If not specified, the file will be placed adjacent to the input file but with different extension.\n"
        "-unify\t Perform CSG union between all brushes of an entity. This reduces the amount of output meshes and polygons.\n"
        "-texroot [folder name]\t Specify a texture root folder relative to cwd (default: \"textures\")\n"
        "-copyright [copyright notice]\t Specify a copyright notice that will be embeded in the exported file.\n"
        "-filter\t Use linear filtering for all textures.\n"
        << std::endl;
}

// recursively print exception whats:
void print_what(const std::exception& e) {
    std::cerr << e.what() << '\n';
    try {
        std::rethrow_if_nested(e);
    }
    catch (const std::exception& nested) {
        std::cerr << "nested: ";
        print_what(nested);
    }
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
    
    bool produceGlb = args.get("glb", false);

    auto optOutputPath = args.get<std::string>("o");
    if (optOutputPath.has_value())
        outputFilePath = optOutputPath.value();
    else
    {
        outputFilePath = inputFilePath;
    
        if (produceGlb)
            outputFilePath.replace_extension(".glb");
        else
            outputFilePath.replace_extension(".gltf");
    }
    
    // std::cout << "Converting: " << inputFilePath << " to " << outputFilePath << "..." << std::endl;
    
    MAPFile mapFile;
    mapFile.unify = args.get<bool>("unify", false);
    mapFile.textureRoot = args.get<std::string>("texRoot", "textures");

    std::vector<Entity> entities;
    std::vector<Texture> textures;
    std::string inputFilePathString = inputFilePath.string();
    bool loaded = mapFile.Load(inputFilePathString.c_str(), entities, textures);
    if (loaded)
    {
        using namespace fx;

        gltf::Document doc;
        doc.asset.generator = "map-to-gltf by Fredrik Lindahl";
        doc.asset.copyright = args.get<std::string>("copyright", {});

        gltf::Scene scene;
        scene.name = "";
            
        int32_t samplerDefaultId = 0;
        { // setup samplers
            gltf::Sampler samplerDefault;
            bool filter = args.get<bool>("filter", false);
            samplerDefault.magFilter = filter ? gltf::Sampler::MagFilter::Linear : gltf::Sampler::MagFilter::Nearest;
            samplerDefault.minFilter = filter ? gltf::Sampler::MinFilter::LinearMipMapLinear : gltf::Sampler::MinFilter::Nearest;
            samplerDefaultId = (int32_t)doc.samplers.size();
            doc.samplers.push_back(std::move(samplerDefault));
        }

        for (auto const& texture : textures)
        {
            gltf::Material mat;
            gltf::Texture tex;
            
            gltf::Image img;
            img.uri = mapFile.textureRoot + "/" + texture.name;
            int32_t const imgId = (int32_t)doc.images.size();

#ifdef _DEBUG
            int32_t const texId = (int32_t)doc.textures.size();
            int32_t const matId = (int32_t)doc.materials.size();
            // Textures and materials in the gltf should be adjacent to the map textures
            assert(imgId == texture.id && texId == texture.id && matId == texture.id);
#endif

            tex.source = imgId;
            tex.name = std::filesystem::path(texture.name).replace_extension("").string();
            tex.sampler = samplerDefaultId;
            
            doc.images.push_back(std::move(img));
            doc.textures.push_back(std::move(tex));

            mat.pbrMetallicRoughness.baseColorTexture = gltf::Material::Texture{ .index{(int32_t)texture.id} };
            mat.name = texture.name;
            mat.doubleSided = false;
            doc.materials.push_back(std::move(mat));
        }

        gltf::Buffer vertexBuffer;

        size_t posOffset = 0;
        size_t normalOffset = 0;
        size_t texOffset = 0;
        size_t indexOffset = 0;
        size_t vertexBufferSize = 0;
        for (auto const& entity : entities)
        {
            for (size_t i = 0; i < entity.primitives.size(); i++)
            {
                size_t const posSize = entity.primitives[i].positionBuffer.size() * sizeof(float);
                size_t const normSize = entity.primitives[i].normalBuffer.size() * sizeof(float);
                size_t const texSize = entity.primitives[i].texcoordBuffer.size() * sizeof(float);
                size_t const indexSize = entity.primitives[i].indexBuffer.size() * sizeof(uint32_t);
                normalOffset += posSize;
                texOffset += posSize + normSize;
                indexOffset += posSize + normSize + texSize;

                vertexBufferSize += posSize;
                vertexBufferSize += normSize;
                vertexBufferSize += texSize;
                vertexBufferSize += indexSize;
            }
        }
        
        vertexBuffer.data.resize(vertexBufferSize);
        vertexBuffer.byteLength = (uint32_t)vertexBufferSize;
        if (!produceGlb)
        {
            std::filesystem::path binaryOutput = outputFilePath;
            binaryOutput.replace_extension(".bin");
            vertexBuffer.uri = binaryOutput.string();
        }
        uint8_t* buffer = vertexBuffer.data.data();
        
        int32_t const vertexBufferIndex = (int32_t)doc.buffers.size();
        doc.buffers.push_back(std::move(vertexBuffer));

        for (auto const& entity : entities)
        {
            gltf::Node node;
            auto nameIt = entity.properties.find("_tb_name");
            if (nameIt != entity.properties.end())
            {
                node.name = nameIt->second;
            }
            else if (entity.properties.size() == 0)
            {
                static uint64_t nodeId = 0;
                node.name = "empty_node_" + std::to_string(nodeId++);
                std::cout << "WARNING: Empty entity detected!" << std::endl;
            }
            
            if (entity.primitives.size() > 0)
            {

                gltf::Mesh mesh;

                for (size_t i = 0; i < entity.primitives.size(); i++)
                {
                    Primitive const& primitive = entity.primitives[i];
                    size_t posNumBytes = primitive.positionBuffer.size() * sizeof(float);
                    size_t normNumBytes = primitive.normalBuffer.size() * sizeof(float);
                    size_t texNumBytes = primitive.texcoordBuffer.size() * sizeof(float);
                    size_t indexNumBytes = primitive.indexBuffer.size() * sizeof(uint32_t);
                    std::memcpy(buffer + posOffset, primitive.positionBuffer.data(), posNumBytes);
                    std::memcpy(buffer + normalOffset, primitive.normalBuffer.data(), normNumBytes);
                    std::memcpy(buffer + texOffset, primitive.texcoordBuffer.data(), texNumBytes);

                    // shift and write indices
                    uint32_t* indexBuffer = (uint32_t*)(buffer + indexOffset);
                    uint32_t const indexShift = posOffset / (sizeof(float) * 3);
                    for (size_t i = 0; i < primitive.indexBuffer.size(); i++)
                    {
                        indexBuffer[i] = primitive.indexBuffer[i] + indexShift;
                    }

                    gltf::BufferView posView;
                    posView.buffer = vertexBufferIndex;
                    posView.byteOffset = (uint32_t)posOffset;
                    posView.byteLength = (uint32_t)posNumBytes;
                    posView.target = gltf::BufferView::TargetType::ArrayBuffer;
                    int32_t const posViewIndex = (int32_t)doc.bufferViews.size();
                    doc.bufferViews.push_back(posView);

                    gltf::BufferView normView;
                    normView.buffer = vertexBufferIndex;
                    normView.byteOffset = (uint32_t)normalOffset;
                    normView.byteLength = (uint32_t)normNumBytes;
                    normView.target = gltf::BufferView::TargetType::ArrayBuffer;
                    int32_t const normViewIndex = (int32_t)doc.bufferViews.size();
                    doc.bufferViews.push_back(normView);

                    gltf::BufferView texView;
                    texView.buffer = vertexBufferIndex;
                    texView.byteOffset = (uint32_t)texOffset;
                    texView.byteLength = (uint32_t)texNumBytes;
                    texView.target = gltf::BufferView::TargetType::ArrayBuffer;
                    int32_t const texViewIndex = (int32_t)doc.bufferViews.size();
                    doc.bufferViews.push_back(texView);

                    gltf::BufferView indexView;
                    indexView.buffer = vertexBufferIndex;
                    indexView.byteOffset = (uint32_t)indexOffset;
                    indexView.byteLength = (uint32_t)indexNumBytes;
                    indexView.target = gltf::BufferView::TargetType::ElementArrayBuffer;
                    int32_t const indexViewIndex = (int32_t)doc.bufferViews.size();
                    doc.bufferViews.push_back(indexView);

                    posOffset += posNumBytes;
                    normalOffset += normNumBytes;
                    texOffset += texNumBytes;
                    indexOffset += indexNumBytes;

                    gltf::Accessor posAccessor;
                    posAccessor.min = { (float)primitive.min.x, (float)primitive.min.y, (float)primitive.min.z };
                    posAccessor.max = { (float)primitive.max.x, (float)primitive.max.y, (float)primitive.max.z };
                    posAccessor.bufferView = posViewIndex;
                    posAccessor.count = (uint32_t)(primitive.positionBuffer.size() / 3);
                    posAccessor.type = gltf::Accessor::Type::Vec3;
                    posAccessor.componentType = gltf::Accessor::ComponentType::Float;

                    gltf::Accessor normalAccessor;
                    normalAccessor.bufferView = normViewIndex;
                    normalAccessor.count = (uint32_t)(primitive.normalBuffer.size() / 3);
                    normalAccessor.type = gltf::Accessor::Type::Vec3;
                    normalAccessor.componentType = gltf::Accessor::ComponentType::Float;

                    gltf::Accessor texAccessor;
                    texAccessor.bufferView = texViewIndex;
                    texAccessor.count = (uint32_t)(primitive.texcoordBuffer.size() / 2);
                    texAccessor.type = gltf::Accessor::Type::Vec2;
                    texAccessor.componentType = gltf::Accessor::ComponentType::Float;

                    gltf::Accessor indexAccessor;
                    indexAccessor.bufferView = indexViewIndex;
                    indexAccessor.count = (uint32_t)(primitive.indexBuffer.size());
                    indexAccessor.type = gltf::Accessor::Type::Scalar;
                    indexAccessor.componentType = gltf::Accessor::ComponentType::UnsignedInt;

                    int32_t const posAccessorIndex = (int32_t)doc.accessors.size();
                    doc.accessors.push_back(posAccessor);
                    int32_t const normalAccessorIndex = (int32_t)doc.accessors.size();
                    doc.accessors.push_back(normalAccessor);
                    int32_t const texAccessorIndex = (int32_t)doc.accessors.size();
                    doc.accessors.push_back(texAccessor);
                    int32_t const indexAccessorIndex = (int32_t)doc.accessors.size();
                    doc.accessors.push_back(indexAccessor);

                    gltf::Primitive gltfPrimitive;
                    gltfPrimitive.mode = gltf::Primitive::Mode::Triangles;
                    gltfPrimitive.material = primitive.textureId;
                    gltfPrimitive.indices = indexAccessorIndex;

                    gltfPrimitive.attributes = {
                        {"POSITION", posAccessorIndex},
                        {"NORMAL", normalAccessorIndex},
                        {"TEXCOORD_0", texAccessorIndex}
                    };

                    mesh.primitives.push_back(gltfPrimitive);
                }

                int32_t const meshIndex = (int32_t)doc.meshes.size();
                doc.meshes.push_back(std::move(mesh));

                node.mesh = meshIndex;
            }
            
            for (auto const& prop : entity.properties)
            {
                node.extensionsAndExtras[prop.first] = prop.second;
            }

            int32_t const nodeId = (int32_t)doc.nodes.size();
            doc.nodes.push_back(std::move(node));
            scene.nodes.push_back(nodeId);
        }

        doc.scenes.push_back(std::move(scene));
        doc.scene = 0;

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
