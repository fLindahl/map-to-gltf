////////////////////////////////////////////////////////////////////
// Filename: map.cpp
//
// Original Author:		Stefan Hajnoczi
//
// Original Date:		5 April 2001
// 
// Description: Code to load .MAP files.
////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <cmath>

#include "map.h"
#include "WAD3.h"


// https://developer.valvesoftware.com/wiki/.map
// https://quakewiki.org/wiki/Quake_Map_Format

MAPFile::Result MAPFile::ParseEntity(Entity** ppEntity_)
{
    if (*ppEntity_ != NULL)
    {
        delete* ppEntity_;
        *ppEntity_ = NULL;
    }

    Brush* pBrushes = NULL;

    Result result = GetToken();

    if (result == RESULT_EOF)
    {
        return RESULT_EOF;
    }
    else if (result == RESULT_FAIL)
    {
        return RESULT_FAIL;
    }

    if (strcmp("{", this->token) != 0)
    {
        std::cout << "Expected:\t{\nFound:\t" << this->token << std::endl;

        return RESULT_FAIL;
    }

    //
    // Parse properties and brushes
    //
    Entity* pEntity = new Entity;

    while (true)
    {
        char c = 0;


        c = fgetc(this->file);
        if (c == EOF)
        {
            std::cout << "File read error!" << std::endl;
            delete pEntity;
            pEntity = NULL;

            return RESULT_FAIL;
        }

        fseek(this->file, -1, SEEK_CUR);

        if (c == '"')
        { // Property
            Property* pProperty = NULL;

            result = ParseProperty(&pProperty);

            if (result != RESULT_SUCCEED)
            {
                std::cout << "Error parsing property!" << std::endl;

                delete pEntity;
                pEntity = NULL;

                return RESULT_FAIL;
            }

            pEntity->AddProperty(pProperty);
        }
        else if (c == '{')
        { // Brush
            Brush* pBrush = NULL;

            result = ParseBrush(&pBrush);

            if (result != RESULT_SUCCEED)
            {
                std::cout << "Error parsing brush!" << std::endl;

                delete pEntity;
                pEntity = NULL;

                return RESULT_FAIL;
            }

            if (pBrushes == NULL)
            {
                pBrushes = pBrush;
            }
            else
            {
                Brush* pTmpBrush = pBrushes;

                while (!pTmpBrush->IsLast())
                {
                    pTmpBrush = pTmpBrush->GetNext();
                }

                pTmpBrush->SetNext(pBrush);
            }
        }
        else if (c == '}')
        { // End of entity

            //
            // Perform CSG union
            //
            if (pBrushes != NULL)
            {
                pEntity->AddPoly(pBrushes->MergeList());

                delete pBrushes;

                pBrushes = NULL;
                this->polygons += pEntity->GetNumberOfPolys();
            }
            /* Do not perform CSG union (useful for debugging)
                        if ( pBrushes != NULL )
                        {
                            Brush *pBrush = pBrushes;

                            while ( pBrush != NULL )
                            {
                                Poly *pPoly = pBrush->GetPolys ( );

                                if ( pPoly != NULL )
                                {
                                    pEntity->AddPoly ( pPoly->CopyList ( ) );
                                }

                                pBrush = pBrush->GetNext ( );
                            }

                            delete pBrushes;

                            pBrushes = NULL;
                            this->polygons += pEntity->GetNumberOfPolys ( );
                        }
            */
            break;
        }
        else
        { // Error
            std::cout << "Expected:\t\", {, or }\nFound:\t" << c << std::endl;

            delete pEntity;
            pEntity = NULL;

            return RESULT_FAIL;
        }
    }

    //
    // Read }
    //
    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading entity!" << std::endl;

        delete pEntity;
        pEntity = NULL;

        return RESULT_FAIL;
    }

    *ppEntity_ = pEntity;
    return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::ParseFace(Face** ppFace_)
{
    //
    // Set up
    //
    if (*ppFace_ != NULL)
    {
        delete* ppFace_;
        *ppFace_ = NULL;
    }

    Face* pFace = new Face;

    //
    // Read plane definition
    //
    Result result;
    Vector3 p[3];

    for (int i = 0; i < 3; i++)
    {
        Vector3 v;

        result = ParseVector(v);

        if (result != RESULT_SUCCEED)
        {
            std::cout << "Error reading plane definition!" << std::endl;

            delete pFace;
            pFace = NULL;

            return RESULT_FAIL;
        }

        p[i] = v;
    }

    pFace->plane.PointsToPlane(p[0], p[1], p[2]);

    //
    // Read texture name
    //
    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading texture name!" << std::endl;

        delete pFace;
        pFace = NULL;

        return RESULT_FAIL;
    }

    Texture* pTexture = NULL;
    int				iWAD = 0;
    bool bFound = false;
    Texture::eGT Result;

    if (this->textureList == NULL)
    {
        this->textureList = new Texture;

        while ((!bFound) && (iWAD < this->wadFiles))
        {
            pTexture = this->textureList->GetTexture(this->token, this->wad[iWAD], this->wadSize[iWAD], Result);

            if (Result == Texture::eGT::GT_LOADED)
            {
                pTexture->uiID = (unsigned int)this->textures;
                this->textures++;

                bFound = true;
            }
            else
            {
                iWAD++;
            }
        }

        this->textureList->SetNext(NULL);

        delete this->textureList;

        this->textureList = pTexture;
    }
    else
    {
        while ((!bFound) && (iWAD < this->wadFiles))
        {
            pTexture = this->textureList->GetTexture(this->token, this->wad[iWAD], this->wadSize[iWAD], Result);

            if (Result == Texture::eGT::GT_LOADED)
            {
                //
                // Texture had to be loaded from the WAD file
                //
                pTexture->uiID = (unsigned int)this->textures;
                this->textures++;

                bFound = true;
            }
            else if (Result == Texture::eGT::GT_FOUND)
            {
                //
                // Texture was already in texture list
                //
                bFound = true;
            }
            else
            {
                iWAD++;
            }
        }
    }

    if (!bFound)
    {
        std::cout << "Unable to find texture " << this->token << "!" << std::endl;

        delete pFace;
        pFace = NULL;

        return RESULT_FAIL;
    }

    pFace->pTexture = pTexture;

    //
    // Read texture axis
    //
    for (size_t i = 0; i < 2; i++)
    {
        Plane p;
        result = ParsePlane(p);

        if (result != RESULT_SUCCEED)
        {
            std::cout << "Error reading texture axis! (Wrong .map version?)" << std::endl;

            delete pFace;
            pFace = NULL;

            return RESULT_FAIL;
        }

        pFace->texAxis[i] = p;
    }

    //
    // Read rotation
    //
    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading rotation!" << std::endl;

        delete pFace;
        pFace = NULL;

        return RESULT_FAIL;
    }

    //
    // Read scale
    //
    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading U scale!" << std::endl;

        delete pFace;
        pFace = NULL;

        return RESULT_FAIL;
    }

    pFace->texScale[0] = atof(this->token) / scale;

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading V scale!" << std::endl;

        delete pFace;
        pFace = NULL;

        return RESULT_FAIL;
    }

    pFace->texScale[1] = atof(this->token) / scale;

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading face!" << std::endl;

        delete pFace;
        pFace = NULL;

        return RESULT_FAIL;
    }

    *ppFace_ = pFace;

    return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::ParseBrush(Brush** ppBrush_)
{
    //
    // Set up
    //
    if (*ppBrush_ != NULL)
    {
        delete* ppBrush_;
        *ppBrush_ = NULL;
    }

    //
    // Read {
    //
    Result result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading brush!" << std::endl;

        return RESULT_FAIL;
    }

    if (strcmp("{", this->token))
    {
        std::cout << "Expected:\t{\nFound:\t" << this->token << std::endl;

        return RESULT_FAIL;
    }

    //
    // Parse brush
    //
    Brush* pBrush = new Brush;
    Face* pFaces = NULL;
    unsigned int uiFaces = 0;


    while (true)
    {
        char c = 0;

        c = fgetc(this->file);
        if (c == EOF)
        {
            std::cout << "Error reading brush!" << std::endl;

            delete pBrush;
            pBrush = NULL;

            if (pFaces)
            {
                delete pFaces;
                pFaces = NULL;
            }

            return RESULT_FAIL;
        }

        fseek(this->file, -1, SEEK_CUR);

        if (c == '(')
        { // Face
            Face* pFace = NULL;

            result = ParseFace(&pFace);

            if (result != RESULT_SUCCEED)
            {
                std::cout << "Error parsing face!" << std::endl;

                delete pBrush;
                pBrush = NULL;

                if (pFaces)
                {
                    delete pFaces;
                    pFaces = NULL;
                }

                return RESULT_FAIL;
            }

            if (pFaces == NULL)
            {
                pFaces = pFace;
            }
            else
            {
                pFaces->AddFace(pFace);
            }

            uiFaces++;
        }
        else if (c == '}')
        { // End of brush
            break;
        }
        else
        {
            std::cout << "Expected:\t( or }\nFound:\t" << c << std::endl;

            delete pBrush;
            pBrush = NULL;

            if (pFaces)
            {
                delete pFaces;
                pFaces = NULL;
            }

            return RESULT_FAIL;
        }
    }

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading brush!" << std::endl;

        delete pBrush;
        pBrush = NULL;

        return RESULT_FAIL;
    }

    Poly* pPolyList = pFaces->GetPolys();

    //
    // Sort vertices and calculate texture coordinates for every polygon
    //
    Poly* pi = pPolyList;
    Face* pFace = pFaces;

    for (unsigned int c = 0; c < uiFaces; c++)
    {
        pi->plane = pFace->plane;
        pi->TextureID = pFace->pTexture->uiID;

        pi->SortVerticesCW();

        pi->CalculateTextureCoordinates(pFace->pTexture->GetWidth(),
            pFace->pTexture->GetHeight(),
            pFace->texAxis, pFace->texScale);

        pFace = pFace->GetNext();
        pi = pi->GetNext();
    }

    pBrush->AddPoly(pPolyList);
    pBrush->CalculateAABB();

    if (pFaces != NULL)
    {
        delete pFaces;
        pFaces = NULL;
    }

    *ppBrush_ = pBrush;

    return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::ParseProperty(Property** ppProperty_)
{
    //
    // Set up
    //
    if (*ppProperty_ != NULL)
    {
        delete* ppProperty_;
        *ppProperty_ = NULL;
    }

    //
    // Read name
    //
    Result result = GetString();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading property name!" << std::endl;

        return RESULT_FAIL;
    }

    Property* pProperty = new Property;

    if (strcmp("mapversion", this->token) == 0)
    {
        //
        // Read value
        //
        result = GetString();

        if (result != RESULT_SUCCEED)
        {
            std::cout << "Error reading value of " << pProperty->GetName() << "!" << std::endl;

            delete pProperty;
            pProperty = NULL;

            return RESULT_FAIL;
        }

        if (strcmp("220", this->token) != 0)
        {
            std::cout << "Wrong map version!" << std::endl;

            delete pProperty;
            return RESULT_FAIL;
        }

        delete pProperty;
        *ppProperty_ = NULL;

        return RESULT_SUCCEED;
    }

    if (strcmp("wad", this->token) == 0)
    {
        pProperty->SetName(this->token);

        //
        // Read value
        //
        result = GetString();

        if (result != RESULT_SUCCEED)
        {
            std::cout << "Error reading value of " << pProperty->GetName() << "!" << std::endl;

            delete pProperty;
            pProperty = NULL;

            return RESULT_FAIL;
        }
        pProperty->SetValue(this->token);
        memset(this->token, 0, MAX_TOKEN_LENGTH + 1);

        const char* pWAD = pProperty->GetValue();
        int iToken = 0;

        /* TODO:
        for (int i = 0; i < strlen(pWAD) + 1; i++)
        {
            if ((pWAD[i] == ';') || (pWAD[i] == 0x00))
            {
                if (this->wad == NULL)
                {
                    this->wad = new LPVOID[this->wadFiles + 1];
                    this->wadSize = new uint32_t[this->wadFiles + 1];
                }
                else
                {
                    LPVOID* pOldWAD = new LPVOID[this->wadFiles];
                    memcpy(pOldWAD, this->wad, sizeof(LPVOID) * (this->wadFiles));
                    delete this->wad;

                    this->wad = new LPVOID[this->wadFiles + 1];
                    memcpy(this->wad, pOldWAD, sizeof(LPVOID) * (this->wadFiles));

                    delete[] pOldWAD;

                    uint32_t* pOldSize = new uint32_t[this->wadFiles];
                    memcpy(pOldSize, this->wadSize, sizeof(uint32_t) * (this->wadFiles));
                    delete this->wadSize;

                    this->wadSize = new uint32_t[this->wadFiles + 1];
                    memcpy(this->wadSize, pOldSize, sizeof(uint32_t) * (this->wadFiles));

                    delete[] pOldSize;
                }

                MapFile(this->token, &this->wad[this->wadFiles], &this->wadSize[this->wadFiles]);

                iToken = 0;
                this->wadFiles++;
                memset(this->token, 0, MAX_TOKEN_LENGTH + 1);
            }
            else
            {
                this->token[iToken] = pWAD[i];
                iToken++;
            }
        }
        */
        * ppProperty_ = pProperty;

        return RESULT_SUCCEED;
    }

    pProperty->SetName(this->token);

    //
    // Read value
    //
    result = GetString();

    if (result != RESULT_SUCCEED)
    {
        std::cout << "Error reading value of " << pProperty->GetName() << "!" << std::endl;

        delete pProperty;
        pProperty = NULL;

        return RESULT_FAIL;
    }

    pProperty->SetValue(this->token);

    *ppProperty_ = pProperty;

    return RESULT_SUCCEED;
}


bool MAPFile::Load(const char* pcFile_, Entity* pEntities_, Texture* pTextures_)
{
    //
    // Check if parameters are valid
    //
    if (pcFile_ == NULL)
    {
        return false;
    }

    if (pEntities_ != NULL)
    {
        delete pEntities_;
        pEntities_ = NULL;
    }

    this->wad = NULL;
    this->wadSize = NULL;
    this->textureList = NULL;

    this->entities = 0;
    this->polygons = 0;
    this->textures = 0;

    this->wadFiles = 0;

    //
    // Open .MAP file
    //
    this->file = fopen(pcFile_, "r");

    if (this->file == nullptr)
    { // Failed to open file
        return false;
    }

    //
    // Parse file
    //
    Entity* pEntityList = NULL;

    // TODO: Add file top info to extras in gltf file.


    while (true)
    {
        // Skip comments
        {
            char buf[2];
            while (true)
            {
                buf[0] = fgetc(this->file);
                buf[1] = fgetc(this->file);
                if (strncmp("//", buf, 2) == 0)
                {
                    while (true)
                    {// seek new line
                        char c = fgetc(this->file);
                        if (c == '\n' || c == '\r')
                        {
                            break;
                        }

                    }
                }
                else
                {
                    // move back 2 characters, since we checked for comments previously and apparently 
                    ungetc(buf[1], this->file);
                    ungetc(buf[0], this->file);
                    break;
                }
            }
        }

        Entity* pEntity = NULL;
        Result result = ParseEntity(&pEntity);

        if (result == RESULT_EOF)
        {
            break;
        }
        else if (result == RESULT_FAIL)
        {
            fclose(this->file);

            if (pEntity != NULL)
            {
                delete pEntity;
                pEntity = NULL;
            }

            if (pEntityList != NULL)
            {
                delete pEntityList;
                pEntityList = NULL;
            }

            for (int i = 0; i < this->wadFiles; i++)
            {
                UnmapViewOfFile(this->wad[i]);
            }

            delete[] this->wad;
            delete[] this->wadSize;
            delete this->textureList;

            return false;
        }

        if (pEntityList == NULL)
        {
            pEntityList = pEntity;
        }
        else
        {
            pEntityList->AddEntity(pEntity);
        }

        this->entities++;
    }

    //
    // Clean up and return
    //
    std::cout << "Entities:\t" << this->entities << std::endl;
    std::cout << "Polygons:\t" << this->polygons << std::endl;
    std::cout << "Textures:\t" << this->textures << std::endl;

    for (int i = 0; i < this->wadFiles; i++)
    {
        UnmapViewOfFile(this->wad[i]);
    }

    delete[] this->wad;
    delete[] this->wadSize;

    fclose(this->file);

    pEntities_ = pEntityList;
    pTextures_ = this->textureList;

    return true;
}


MAPFile::Result MAPFile::ParsePlane(Plane& p_)
{
    Result result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    if (strcmp("[", this->token))
    {
        return RESULT_FAIL;
    }

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    p_.n.x = atof(this->token);

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    p_.n.z = atof(this->token);

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    p_.n.y = atof(this->token);

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    p_.d = atof(this->token);

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    if (strcmp("]", this->token))
    {
        return RESULT_FAIL;
    }

    return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::ParseVector(Vector3& v_)
{
    Result result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    if (strcmp("(", this->token))
    {
        return RESULT_FAIL;
    }

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    v_.x = atof(this->token) / scale;

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    v_.z = atof(this->token) / scale;

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    v_.y = atof(this->token) / scale;

    result = GetToken();

    if (result != RESULT_SUCCEED)
    {
        return RESULT_FAIL;
    }

    if (strcmp(")", this->token))
    {
        return RESULT_FAIL;
    }

    return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::GetToken()
{
    unsigned int i = 0;
    char c = 0;
    memset(&this->token, 0, sizeof(this->token));

    while (i <= MAX_TOKEN_LENGTH)
    {
        c = fgetc(this->file);
        if (c == EOF)
        {
            if (feof(this->file))
                return RESULT_EOF;
            else
                return RESULT_FAIL;
        }

        //
        // Check for token end
        //
        if (c == ' ' || c == '\n' || c == '\r')
        {
            break;
        }

        this->token[i] = c;
        
        i++;
    }

    return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::GetString()
{
    unsigned int i = 0;
    char c = 0;
    bool bFinished = false;

    memset(&this->token, 0, sizeof(this->token));

    //
    // Read first "
    //
    c = fgetc(this->file);
    if (c == EOF)
    {
        if (feof(this->file))
            return RESULT_EOF;
        else
            return RESULT_FAIL;
    }

    //
    // Parse rest of string
    //
    while (i <= MAX_TOKEN_LENGTH)
    {
        c = fgetc(this->file);
        if (c == EOF)
        {
            return RESULT_FAIL;
        }

        //
        // Check for token end
        //
        if (c == '"')
        {
            bFinished = true;
        }

        if (bFinished && (c == ' '))
        {
            break;
        }

        if (bFinished && (c == 0x0A))
        {
            break;
        }

        if (!bFinished)
        {
            this->token[i] = c;
        }

        i++;
    }

    return RESULT_SUCCEED;
}