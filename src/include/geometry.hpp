#pragma once

#include <string>
#include <string_view>
#include <vector>

struct stringstream;
#include "../maths/vec.hpp"
// Anything following a hash is a commnet -> Says wikipedia

// Let's start loading OBJ 3D objects 

// It should describe the structure of the 3D object

struct Face
{
    // Face will store all the parameters required 
    std::vector<Vec4f> vertices; 
    std::vector<Vec2f> texCoord; 
};

namespace Pipeline3D
{
struct VertexAttrib3D;
}


struct Object3D
{
    std::vector<Vec4f> Vertices{};
    std::vector<Vec2f> TextureCoords{};

    std::vector<Face> Faces; 
    
    Object3D(std::string_view obj_path);

    void ParseOBJ(std::stringstream& str); 
    void LoadGeometry(std::vector<Pipeline3D::VertexAttrib3D>&);
};
