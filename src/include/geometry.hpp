#pragma once

#include <string>
#include <string_view>
#include <vector>

struct stringstream;
#include "../maths/vec.hpp"
// Anything following a hash is a commnet -> Says wikipedia

// Let's start loading OBJ 3D objects

// It should describe the structure of the 3D object
// TODO :: Create model vertices and texture hierarchy 

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

struct Material
{
    std::string material_name; // As specified in the obj file
    std::string material_path;
    uint32_t    texture_id;
    Vec3f       ambient;
    Vec3f       diffuse;
    Vec3f       specular;

    float       shiny;
    float       dissolve;
    float       opticalDensity;

    std::string specular_map;
    std::string diffuse_map;
};

struct Object3D
{
    // so maybe replace these things with vectors of vectors to allow multiple materials
    // so each vector of vertices relates to each vector of material 
    std::vector<Vec4f>    Vertices{};
    std::vector<Material> Materials{};
    std::vector<Vec2f>    TextureCoords{};

    std::vector<Face>     Faces;
    Object3D(std::string_view obj_path);

    void ParseOBJ(std::stringstream &str, std::string_view current_path);
    void ParseMaterials(std::string_view current_path);
    void LoadGeometry(std::vector<Pipeline3D::VertexAttrib3D> &vertices, std::vector<uint32_t> &indices);
};
