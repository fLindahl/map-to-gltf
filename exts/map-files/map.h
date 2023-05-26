#pragma once

////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////

const unsigned int MAX_TOKEN_LENGTH = 512;
const unsigned int MAX_TEXTURE_LENGTH = 16;


////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "math.h"
#include "entity.h"
#include "brush.h"


////////////////////////////////////////////////////////////////////
// Classes
////////////////////////////////////////////////////////////////////

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

    Result ParseEntity(MapEntity& entity);
    Result ParseProperty(std::pair<PropertyName, PropertyValue>& prop);
    Result ParseBrush(MapBrush& brush);
    Result ParseFace(MapFace& face);
    Result ParseVector(Vector3& v_);
    Result ParsePlane(Plane& p_);

    std::vector<MapEntity>* mapEntities;
    std::vector<MapTexture>* mapTextures;
    std::unordered_map<std::string, uint32_t> textureTable;
    std::vector<std::string> textureLibs;

public:

    bool unify;
    std::string textureRoot;

    bool Load(const char* pcFile_, std::vector<MapEntity>& entities, std::vector<MapTexture>& textures);
};