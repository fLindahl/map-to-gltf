//------------------------------------------------------------------------------
//  @file mapconverter.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "exts/stb/stb_image_write.h"
#include "exts/stb/stbimage.h"

#include "mapconverter.h"
#include "math.h"

//------------------------------------------------------------------------------
/**
*/
nlohmann::json
ConvertProperty(PropertyName name, PropertyValue prop, float meshScale, bool useLH)
{
    nlohmann::json ret;
    std::vector<float> v;
    std::istringstream ss(prop);
    std::istream_iterator <float> it(ss);
    std::istream_iterator <float> eos;
    while (it != eos)
    {
        v.push_back(*it);
        it++;
    };

    if (v.empty())
    {
        ret = prop; // probably a string
    }
    else if (v.size() == 1)
    {
        ret = v[0]; // int or float
    }
    else
    {
        // check for special cases
        if (name == "_color" || name == "angles")
        {
            // do nothing with it.
        }
        else
        {
            // adjust vector if it's a 3D vector, since they usually represent a point. 
            if (v.size() == 3)
            {
                float x = v[0];
                float y = v[1];
                float z = v[2];

                // Flip x axis as well, if we're using RH system
                // output XZY, since Z is up in Trenchbroom
                float flip = (-1.0f + (float)useLH);
                v[0] = (x / (float)scale) * meshScale * flip;
                v[1] = (z / (float)scale) * meshScale;
                v[2] = (y / (float)scale) * meshScale;
            }
        }
        ret = v; // vector of floats
    }

    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
MapConverter::CreateNodes(fx::gltf::Document& doc, std::vector<Entity> const& entities)
{
    using namespace fx;

    gltf::Scene& scene = doc.scenes[doc.scene];

    for (Entity const& entity : entities)
    {
        int32_t const nodeId = (int32_t)doc.nodes.size();
        doc.nodes.push_back(gltf::Node());
        gltf::Node& node = doc.nodes.back();
        scene.nodes.push_back(nodeId);

        auto nameIt = entity.properties.find("_tb_name");
        if (nameIt != entity.properties.end())
        {
            node.name = nameIt->second;
        }
        else if (entity.properties.size() == 0)
        {
            node.name = "empty_node_" + std::to_string(nodeId);
            std::cout << "WARNING: Empty entity detected!" << std::endl;
        }
        else
        {
            if (entity.properties.contains("classname"))
            {
                node.name = "unnamed_" + entity.properties.at("classname") + "_" + std::to_string(nodeId);
            }
            else
            {
                node.name = "unnamed_node_" + std::to_string(nodeId);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MapConverter::CreateMeshes(
    fx::gltf::Document& doc, 
    std::vector<Entity> const& entities, 
    bool produceGlb, 
    std::filesystem::path const& outputFilePath
)
{
    using namespace fx;
    gltf::Buffer meshBuffer;

    size_t posOffset = 0;
    size_t normalOffset = 0;
    size_t texOffset = 0;
    size_t indexOffset = 0;
    size_t indexNumBytes = 0;
    size_t posNumBytes = 0;
    size_t normalNumBytes = 0;
    size_t texNumBytes = 0;
    size_t bufferTotalNumBytes = 0;
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

            posNumBytes += posSize;
            normalNumBytes += normSize;
            texNumBytes += texSize;
            indexNumBytes += indexSize;

            bufferTotalNumBytes += posSize;
            bufferTotalNumBytes += normSize;
            bufferTotalNumBytes += texSize;
            bufferTotalNumBytes += indexSize;
        }
    }

    meshBuffer.data.resize(bufferTotalNumBytes);
    meshBuffer.byteLength = (uint32_t)bufferTotalNumBytes;
    if (!produceGlb)
    {
        std::filesystem::path binaryOutput = outputFilePath;
        binaryOutput.replace_extension(".bin");
        meshBuffer.uri = binaryOutput.string();
    }
    uint8_t* buffer = meshBuffer.data.data();

    int32_t const vertexBufferIndex = (int32_t)doc.buffers.size();
    doc.buffers.push_back(std::move(meshBuffer));

    gltf::BufferView posView;
    posView.buffer = vertexBufferIndex;
    posView.byteOffset = (uint32_t)posOffset;
    posView.byteLength = (uint32_t)posNumBytes;
    posView.byteStride = sizeof(float) * 3;
    posView.target = gltf::BufferView::TargetType::ArrayBuffer;
    int32_t const posViewIndex = (int32_t)doc.bufferViews.size();
    doc.bufferViews.push_back(posView);

    gltf::BufferView normView;
    normView.buffer = vertexBufferIndex;
    normView.byteOffset = (uint32_t)normalOffset;
    normView.byteLength = (uint32_t)normalNumBytes;
    normView.byteStride = sizeof(float) * 3;
    normView.target = gltf::BufferView::TargetType::ArrayBuffer;
    int32_t const normViewIndex = (int32_t)doc.bufferViews.size();
    doc.bufferViews.push_back(normView);

    gltf::BufferView texView;
    texView.buffer = vertexBufferIndex;
    texView.byteOffset = (uint32_t)texOffset;
    texView.byteLength = (uint32_t)texNumBytes;
    texView.byteStride = sizeof(float) * 2;
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

    uint32_t accessorPosOffset = 0;
    uint32_t accessorNormalOffset = 0;
    uint32_t accessorTexOffset = 0;
    uint32_t accessorIndexOffset = 0;

    // setup primitives

    for (int32_t nodeId = 0; nodeId < entities.size(); nodeId++)
    {
        Entity const& entity = entities.at(nodeId);
        gltf::Node& node = doc.nodes[nodeId];
        bool const isPointEntity = (entity.primitives.size() == 0);

        if (!isPointEntity)
        {
            gltf::Mesh mesh;
            mesh.name = node.name + "_mesh";

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
                std::memcpy(buffer + indexOffset, primitive.indexBuffer.data(), indexNumBytes);

                posOffset += posNumBytes;
                normalOffset += normNumBytes;
                texOffset += texNumBytes;
                indexOffset += indexNumBytes;

                gltf::Accessor posAccessor;
                posAccessor.min = { (float)primitive.min.x, (float)primitive.min.y, (float)primitive.min.z };
                posAccessor.max = { (float)primitive.max.x, (float)primitive.max.y, (float)primitive.max.z };
                posAccessor.bufferView = posViewIndex;
                posAccessor.count = (uint32_t)(primitive.positionBuffer.size() / 3);
                posAccessor.byteOffset = accessorPosOffset;
                posAccessor.type = gltf::Accessor::Type::Vec3;
                posAccessor.componentType = gltf::Accessor::ComponentType::Float;

                gltf::Accessor normalAccessor;
                normalAccessor.bufferView = normViewIndex;
                normalAccessor.count = (uint32_t)(primitive.normalBuffer.size() / 3);
                normalAccessor.type = gltf::Accessor::Type::Vec3;
                normalAccessor.byteOffset = accessorNormalOffset;
                normalAccessor.componentType = gltf::Accessor::ComponentType::Float;

                gltf::Accessor texAccessor;
                texAccessor.bufferView = texViewIndex;
                texAccessor.count = (uint32_t)(primitive.texcoordBuffer.size() / 2);
                texAccessor.type = gltf::Accessor::Type::Vec2;
                texAccessor.byteOffset = accessorTexOffset;
                texAccessor.componentType = gltf::Accessor::ComponentType::Float;

                gltf::Accessor indexAccessor;
                indexAccessor.bufferView = indexViewIndex;
                indexAccessor.count = (uint32_t)(primitive.indexBuffer.size());
                indexAccessor.type = gltf::Accessor::Type::Scalar;
                indexAccessor.byteOffset = accessorIndexOffset;
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

                accessorPosOffset += (uint32_t)posNumBytes;
                accessorNormalOffset += (uint32_t)normNumBytes;
                accessorTexOffset += (uint32_t)texNumBytes;
                accessorIndexOffset += (uint32_t)indexNumBytes;
            }

            int32_t const meshIndex = (int32_t)doc.meshes.size();
            doc.meshes.push_back(std::move(mesh));

            node.mesh = meshIndex;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MapConverter::SetupProperties(fx::gltf::Document& doc, std::vector<Entity> const& entities, float meshScale, bool useLH)
{
    using namespace fx;

    for (int32_t nodeId = 0; nodeId < entities.size(); nodeId++)
    {
        Entity const& entity = entities.at(nodeId);
        gltf::Node& node = doc.nodes[nodeId];
        bool const isPointEntity = (entity.primitives.size() == 0);

        const std::string originName = "origin";
        if (entity.properties.contains(originName))
        {
            std::vector<float> origin = ConvertProperty(originName, entity.properties.at(originName), meshScale, useLH).get<std::vector<float>>();
            node.translation = { origin[0], origin[1], origin[2] };
        }
        else
        {
            node.translation = { (float)entity.origin.x, (float)entity.origin.y, (float)entity.origin.z };
        }

        if (isPointEntity)
        {
            // if it's not a point entity, the rotations aren't necessary since they're already baked into the mesh.
            const std::string anglesName = "angles";
            if (entity.properties.contains(anglesName))
            {
                std::vector<float> angles = ConvertProperty(anglesName, entity.properties.at(anglesName), meshScale, useLH).get<std::vector<float>>();
                node.rotation = QuatFromEuler(angles[2], angles[1] + 90.0f, angles[0]);
            }
            else
            {
                // default rotation is 90deg around y axis, since "X is forward" is default for the MAP format.
                constexpr std::array<float, 4> rot = { 0, 0.7071068f, 0, 0.7071068f };
                node.rotation = rot;
            }
        }

        for (auto const& prop : entity.properties)
        {
            nlohmann::json var = ConvertProperty(prop.first, prop.second, meshScale, useLH);
            node.extensionsAndExtras["extras"][prop.first] = var;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MapConverter::GeneratePhysicsNodes(fx::gltf::Document& doc, std::vector<Entity> const& entities)
{
    using namespace fx;

    if (std::find(doc.extensionsUsed.begin(), doc.extensionsUsed.end(), "OMI_collider") == doc.extensionsUsed.end())
    {
        doc.extensionsUsed.push_back("OMI_collider");
    }

    gltf::Scene& scene = doc.scenes[doc.scene];

    size_t const numEntities = entities.size();
    for (size_t nodeId = 0; nodeId < numEntities; nodeId++)
    {
        Entity const& entity = entities.at(nodeId);
        gltf::Node& node = doc.nodes[nodeId];

        // Create physics node if necessary
        if (entity.physics.shape != Physics::Shape::None)
        {
            gltf::Node physicsNode;
            physicsNode.name = node.name + "_physics";
            physicsNode.extensionsAndExtras["extensions"]["OMI_collider"]["type"] = Physics::ShapeName(entity.physics.shape);
            if (entity.physics.shape == Physics::Shape::AABB)
            {
                Vector3 const size = entity.bboxMax - entity.bboxMin;
                physicsNode.extensionsAndExtras["extensions"]["OMI_collider"]["size"] = { size.x, size.y, size.z };
                physicsNode.translation = {
                    (float)entity.physics.center.x,
                    (float)entity.physics.center.y,
                    (float)entity.physics.center.z
                };
            }
            if (entity.physics.shape == Physics::Shape::Hull ||
                entity.physics.shape == Physics::Shape::TriMesh)
            {
                physicsNode.extensionsAndExtras["extensions"]["OMI_collider"]["mesh"] = node.mesh;
            }

            unsigned int const physicsNodeId = (unsigned int)doc.nodes.size();
            doc.nodes.push_back(std::move(physicsNode));
            scene.nodes.push_back(physicsNodeId);

            node.children.push_back(physicsNodeId);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
MapConverter::CreateTextures(fx::gltf::Document& doc, std::vector<Texture> const& textures, bool filter, bool embedImages, std::string const& textureRoot)
{
    using namespace fx;

    int32_t samplerDefaultId = 0;
    { // setup samplers
        gltf::Sampler samplerDefault;
        samplerDefault.magFilter = filter ? gltf::Sampler::MagFilter::Linear : gltf::Sampler::MagFilter::Nearest;
        samplerDefault.minFilter = filter ? gltf::Sampler::MinFilter::LinearMipMapLinear : gltf::Sampler::MinFilter::Nearest;
        samplerDefaultId = (int32_t)doc.samplers.size();
        doc.samplers.push_back(std::move(samplerDefault));
    }

    gltf::Buffer* imgBuffer = nullptr; // only used if embedded images
    uint32_t imgBufferId;
    if (embedImages)
    {
        gltf::Buffer newBuffer; // only used if embedded images

        imgBufferId = (uint32_t)doc.buffers.size();
        doc.buffers.push_back(newBuffer);
        imgBuffer = &doc.buffers[imgBufferId];
        imgBuffer->name = "embedded_images";
    }

    for (auto const& texture : textures)
    {
        gltf::Material mat;
        gltf::Texture tex;

        gltf::Image img;
        std::string source = textureRoot + texture.name;

        if (embedImages)
        {
            int bufferViewLen = -1;
            int x, y, n;
            unsigned char* loadData = stbi_load(source.c_str(), &x, &y, &n, 0);
            int offset;
            if (loadData != nullptr)
            {
                // convert data to png, thus we can support other formats than just png and jpg
                // TODO: if the file is already jpg or png, we can just store it directly.
                unsigned char* pngData = stbi_write_png_to_mem(loadData, 0, x, y, n, &bufferViewLen);

                for (int i = 0; i < bufferViewLen; i++)
                {
                    imgBuffer->data.push_back(pngData[i]);
                }
                offset = imgBuffer->byteLength;
                imgBuffer->byteLength += (uint32_t)bufferViewLen * sizeof(char);
                free(pngData);
                stbi_image_free(loadData);
            }
            else
            {
                std::cerr << "ERROR: Image '" << source << "' not found!" << std::endl;
                return;
            }

            gltf::BufferView view;
            view.name = source;
            view.buffer = imgBufferId;
            view.byteLength = bufferViewLen;
            view.byteOffset = offset;

            uint32_t bufferViewId = (uint32_t)doc.bufferViews.size();
            doc.bufferViews.push_back(view);

            img.bufferView = bufferViewId;
            img.mimeType = "image/png";
        }
        else
        {
            img.uri = source;
        }
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
        mat.pbrMetallicRoughness.metallicFactor = 0.0f;
        mat.name = texture.name;
        mat.doubleSided = false;
        doc.materials.push_back(std::move(mat));
    }

    if (imgBuffer)
        imgBuffer->SetEmbeddedResource();
}
