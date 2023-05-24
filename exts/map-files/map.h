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

    int wadFiles;
    void** wad;
    uint32_t* wadSize;
    Texture* textureList;

    int entities;
    int polygons;
    int textures;

    Result GetToken();
    Result GetString();
    Result SkipComments();

    Result ParseEntity(Entity** ppEntity_);
    Result ParseProperty(Property** ppProperty_);
    Result ParseBrush(Brush** ppBrush_);
    Result ParseFace(Face** ppFace_);
    Result ParseVector(Vector3& v_);
    Result ParsePlane(Plane& p_);

public:
    bool Load(const char* pcFile_, Entity* ppEntities_, Texture* pTexture_);
};