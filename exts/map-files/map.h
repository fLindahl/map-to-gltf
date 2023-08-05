#pragma once

const unsigned int MAX_TOKEN_LENGTH = 512;
const unsigned int MAX_TEXTURE_LENGTH = 16;

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <cstdint>

#include "math.h"
#include "entity.h"
#include "brush.h"

class MAPFile
{
private:
    enum Result
    {
        RESULT_SUCCEED = 0,
        RESULT_FAIL,
        RESULT_EOF
    };

    char token[MAX_TOKEN_LENGTH + 1];

    FILE* file;

    Result GetToken();
    Result GetString();
    Result SkipComments();

    Result ParseEntity(Entity& entity);
    Result ParseProperty(std::pair<PropertyName, PropertyValue>& prop);
    Result ParseBrush(Brush& brush);
    Result ParseFace(Face& face);
    Result ParseVector(Vector3& v_);
    Result ParsePlane(Plane& p_);

    std::vector<Entity>* mapEntities;
    std::vector<Texture>* mapTextures;
    std::unordered_map<std::string, uint32_t> textureTable;
    std::vector<std::string> textureLibs;

public:

    bool unify;
    std::string textureRoot;
    float meshScale = 1.0f;
    bool useLH = false;

    bool Load(const char* pcFile_, std::vector<Entity>& entities, std::vector<Texture>& textures);
};