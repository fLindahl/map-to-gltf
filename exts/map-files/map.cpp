////////////////////////////////////////////////////////////////////
// Filename: map.cpp
//
// Original Author:		Stefan Hajnoczi
// Original Date:		5 April 2001
// 
// Heavily modified by Fredrik Lindahl
// 
// Description: Code to load .MAP files.
////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <cmath>
#include <filesystem>

#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stbimage.h"

#include "map.h"


// https://developer.valvesoftware.com/wiki/.map
// https://quakewiki.org/wiki/Quake_Map_Format

MAPFile::Result MAPFile::ParseEntity(Entity& entity)
{
	std::vector<Brush> brushes;
	brushes.reserve(128);

	Result result = GetToken();

	if (result != RESULT_SUCCEED)
		return result;
	
	if (strcmp("{", this->token) != 0)
	{
		std::cout << "Expected:\t{\nFound:\t" << this->token << std::endl;
		return RESULT_FAIL;
	}

	// Parse properties and brushes
	while (true)
	{
		SkipComments();

		char c = 0;
		c = fgetc(this->file);
		if (c == EOF)
		{
			std::cout << "File read error!" << std::endl;
			return RESULT_FAIL;
		}

		fseek(this->file, -1, SEEK_CUR);

		if (c == '"')
		{ // Property
			std::pair<PropertyName, PropertyValue> prop;

			result = ParseProperty(prop);

			if (result != RESULT_SUCCEED)
			{
				std::cout << "Error parsing property!" << std::endl;
				return RESULT_FAIL;
			}

			entity.properties.emplace(prop);
		}
		else if (c == '{')
		{ // Brush
			Brush brush;

			result = ParseBrush(brush);

			if (result != RESULT_SUCCEED)
			{
				std::cout << "Error parsing brush!" << std::endl;
				return RESULT_FAIL;
			}
			
			brushes.push_back(brush);
		}
		else if (c == '}')
		{ // End of entity
			if (!brushes.empty())
			{
				std::vector<Poly> polygons;
				if (this->unify)
				{
					std::vector<Poly> polygons = CSG::Union(brushes);
				}
				else
				{
					// Do not perform CSG union (useful for debugging)
					for (auto const& brush : brushes)
					{
						polygons.insert(polygons.end(), brush.polys.begin(), brush.polys.end());
						
					}
				}

				entity.polys.insert(entity.polys.end(), polygons.begin(), polygons.end());
			}
			break;
		}
		else
		{ // Error
			std::cout << "Expected:\t\", {, or }\nFound:\t" << c << std::endl;
			return RESULT_FAIL;
		}
	}

	// Read }
	result = GetToken();

	if (result != RESULT_SUCCEED)
	{
		std::cout << "Error reading entity!" << std::endl;
		return RESULT_FAIL;
	}

	entity.primitives = GeneratePrimitives(entity.polys);

	return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::ParseFace(Face& face)
{
	// Read plane definition
	Result result;
	Vector3 p[3];

	for (int i = 0; i < 3; i++)
	{
		Vector3 v;

		result = ParseVector(v);

		if (result != RESULT_SUCCEED)
		{
			std::cout << "Error reading plane definition!" << std::endl;
			return RESULT_FAIL;
		}

		p[i] = v;
	}

	face.plane.PointsToPlane(p[0], p[1], p[2]);
	
	// Read texture name
	result = GetToken();

	if (result != RESULT_SUCCEED)
	{
		std::cout << "Error reading texture name!" << std::endl;
		return RESULT_FAIL;
	}

	bool foundTexture = false;
	
	uint32_t textureId = 0xFFFFFFFF;
	auto id = this->textureTable.find(this->token);
	if (id != this->textureTable.end())
	{
		textureId = id->second;
		foundTexture = true;
	}
	else
	{
		// Try to find texture in any of the texture folders
		
		const char* supportedExts[3] = { ".png", ".jpg", ".bmp" };

		for (size_t i = 0; i < sizeof(supportedExts) / sizeof(const char*); i++)
		{
			std::string fullRelPath = this->textureRoot + "/" + this->token + supportedExts[i];
			if (std::filesystem::exists(fullRelPath))
			{
				int x,y,n;
				if (stbi_info(fullRelPath.c_str(), &x, &y, &n))
				{
					Texture texture;
					texture.id = static_cast<uint32_t>(this->mapTextures->size());
					texture.width = x;
					texture.height = y;
					texture.name = std::string(this->token) + supportedExts[i];

					this->textureTable[std::string(this->token)] = texture.id;
					this->mapTextures->push_back(texture);

					textureId = texture.id;
					foundTexture = true;
					break;
				}
			}
		}
	}

	if (!foundTexture)
	{
		std::cout << "Unable to find texture " << this->token << "!" << std::endl;
		return RESULT_FAIL;
	}

	face.textureId = textureId;

	// Read texture axis
	for (size_t i = 0; i < 2; i++)
	{
		Plane p;
		result = ParsePlane(p);

		if (result != RESULT_SUCCEED)
		{
			std::cout << "Error reading texture axis! (Wrong .map version?)" << std::endl;
			return RESULT_FAIL;
		}

		face.texAxis[i] = p;
	}

	// Read rotation
	result = GetToken();

	if (result != RESULT_SUCCEED)
	{
		std::cout << "Error reading rotation!" << std::endl;
		return RESULT_FAIL;
	}

	// No need to do anything with rotation since it's already 
	// applied to the texture axis

	// Read scale
	result = GetToken();

	if (result != RESULT_SUCCEED)
	{
		std::cout << "Error reading U scale!" << std::endl;
		return RESULT_FAIL;
	}

	face.texScale[0] = atof(this->token) / scale;

	result = GetToken();

	if (result != RESULT_SUCCEED)
	{
		std::cout << "Error reading V scale!" << std::endl;
		return RESULT_FAIL;
	}

	face.texScale[1] = atof(this->token) / scale;

	return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::ParseBrush(Brush& brush)
{
	// Read {
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

	// Parse brush
	std::vector<Face> faces;

	while (true)
	{
		char c = 0;

		c = fgetc(this->file);
		if (c == EOF)
		{
			std::cout << "Error reading brush!" << std::endl;
			return RESULT_FAIL;
		}

		fseek(this->file, -1, SEEK_CUR);

		if (c == '(')
		{ // Face
			Face face;

			result = ParseFace(face);

			if (result != RESULT_SUCCEED)
			{
				std::cout << "Error parsing face!" << std::endl;
				return RESULT_FAIL;
			}

			faces.push_back(face);
		}
		else if (c == '}')
		{ // End of brush
			break;
		}
		else
		{
			std::cout << "Expected:\t( or }\nFound:\t" << c << std::endl;
			return RESULT_FAIL;
		}
	}

	result = GetToken();

	if (result != RESULT_SUCCEED)
	{
		std::cout << "Error reading brush!" << std::endl;
		return RESULT_FAIL;
	}

	std::vector<Poly> polys = DerivePolys(faces);

	if (polys.size() != faces.size())
	{
		std::cout << "Error reading brush! Num polys was different from num faces!" << std::endl;
		return RESULT_FAIL;
	}

	// Sort vertices and calculate texture coordinates for every polygon
	for (size_t i = 0; i < polys.size(); i++)
	{
		Poly& poly = polys[i];
		Face const& face = faces[i];

		poly.plane = face.plane;
		poly.textureId = face.textureId;

		poly.SortVerticesCW();

		poly.CalculateTextureCoordinates(
			this->mapTextures->at(face.textureId).width,
			this->mapTextures->at(face.textureId).height,
			face.texAxis,
			face.texScale
		);
	}

	brush.polys.insert(brush.polys.end(), polys.begin(), polys.end());
	brush.CalculateAABB();

	return RESULT_SUCCEED;
}


MAPFile::Result MAPFile::ParseProperty(std::pair<PropertyName, PropertyValue>& prop)
{
	//
	// Read name
	//
	Result result = GetString();

	if (result != RESULT_SUCCEED)
	{
		std::cout << "Error reading property name!" << std::endl;
		return RESULT_FAIL;
	}

	prop.first = this->token;

	if (strcmp("mapversion", this->token) == 0)
	{
		// Read value
		result = GetString();

		if (result != RESULT_SUCCEED)
		{
			std::cout << "Error reading value of property 'mapversion'!" << std::endl;

			return RESULT_FAIL;
		}

		if (strcmp("220", this->token) != 0)
		{
			std::cout << "Wrong map version!" << std::endl;
			return RESULT_FAIL;
		}

		prop.second = this->token;

		return RESULT_SUCCEED;
	}

	if (strcmp("wad", this->token) == 0)
	{
		// Read value
		result = GetString();

		if (result != RESULT_SUCCEED)
		{
			std::cout << "Error reading value of property 'wad'!" << std::endl;
			return RESULT_FAIL;
		}

		prop.second = this->token;
		memset(this->token, 0, MAX_TOKEN_LENGTH + 1);

		std::string_view const wads = prop.second;
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

		return RESULT_SUCCEED;
	}

	if (strcmp("_tb_textures", this->token) == 0)
	{
		// Read value
		result = GetString();

		if (result != RESULT_SUCCEED)
		{
			std::cout << "Error reading value of property '_tb_textures'!" << std::endl;
			return RESULT_FAIL;
		}

		prop.second = this->token;
		
		char* tok = strtok(this->token, ";");
		
		while (tok != NULL)
		{
			std::string path = tok;
			
			// This is kinda dumb, but Trenchbroom extracts the first folder from the textures when listing them per plane. 
			// Remove the first directory from all paths.
			size_t position = path.find("/");
			if (position == std::string::npos)
				position = path.find("\\");

			if (position == std::string::npos)
				path = ""; // root path. Just add an empty string
			else
			{
				if (position + 1 > path.length())
				{
					// Last character of path is directory delimiter. This shouldn't happen AFAIK.
					return RESULT_FAIL;
				}
				path = path.substr(position + 1);
			}

			this->textureLibs.push_back(std::move(path));
			tok = strtok(NULL, ";");
		}

		return RESULT_SUCCEED;
	}

	memset(this->token, 0, MAX_TOKEN_LENGTH + 1);

	//
	// Read value
	//
	result = GetString();

	if (result != RESULT_SUCCEED)
	{
		std::cout << "Error reading value of " << prop.first << "!" << std::endl;
		return RESULT_FAIL;
	}

	prop.second = this->token;
	return RESULT_SUCCEED;
}


bool MAPFile::Load(const char* mapFilePath, std::vector<Entity>& entities, std::vector<Texture>& textures)
{
	// Check if parameters are valid
	if (mapFilePath == NULL)
	{
		return false;
	}

	// Open .MAP file
	this->file = fopen(mapFilePath, "r");

	if (this->file == nullptr)
	{ // Failed to open file
		return false;
	}

	// Parse file
	// TODO: Add file top info to extras in gltf file.

	this->mapEntities = &entities;
	this->mapTextures = &textures;

	while (true)
	{
		SkipComments();

		Entity entity;
		Result result = ParseEntity(entity);

		if (result == RESULT_EOF)
		{
			break;
		}
		else if (result == RESULT_FAIL)
		{
			fclose(this->file);
			return false;
		}

		entities.push_back(std::move(entity));
	}

	//
	// Clean up and return
	//
	std::cout << "Entities:\t" << entities.size() << std::endl;
	//std::cout << "Polygons:\t" << this->polygons << std::endl;
	//std::cout << "Textures:\t" << this->textures << std::endl;

	fclose(this->file);

	this->mapEntities = nullptr;
	this->mapTextures = nullptr;

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

MAPFile::Result MAPFile::SkipComments()
{
	// Skip comments
	{
		char buf[2];
		while (true)
		{
			for (int i = 0; i < 2; i++)
			{
				buf[i] = fgetc(this->file);
				if (buf[i] == EOF)
				{
					if (feof(this->file))
						return RESULT_EOF;
					else
						return RESULT_FAIL;
				}
			}

			if (strncmp("//", buf, 2) == 0)
			{
				while (true)
				{// seek new line
					char c = fgetc(this->file);
					if (c == EOF)
					{
						if (feof(this->file))
							return RESULT_EOF;
						else
							return RESULT_FAIL;
					}
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

	return Result::RESULT_SUCCEED;
}
