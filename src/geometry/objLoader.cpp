#include "../include/geometry.hpp"
#include "../include/rasteriser.h"

#include <format>
#include <fstream>
#include <sstream>

Object3D::Object3D(std::string_view obj_path)
{
    std::ifstream objfile(obj_path.data(), std::ios::binary | std::ios::in);
    if (!objfile.is_open())
        std::cerr << "Failed to open file " << obj_path << std::endl;
    else
    {
        std::stringstream buffer;
        auto              len = obj_path.find_last_of('/');
        std::string       current_path;
        if (len == std::string::npos)
            // There's no / here .. so it should be in the current directory
            current_path = std::string("./");
        else
            current_path = obj_path.substr(0, len + 1);
        buffer << objfile.rdbuf();
        ParseOBJ(buffer, current_path);
    }
}

void Object3D::ParseOBJ(std::stringstream &str, std::string_view current_path)
{
    std::string ch;
    str >> ch;
    while (!str.eof())
    {
        if (ch == "mtllib")
        {
            // Read another string which is the name of the file containing the material
            Material mat;
            str >> mat.material_path;
            Materials.push_back(mat);
        }
        else if (ch == "v")
        {
            Vec4f vertex;
            str >> vertex.x;
            str >> vertex.y;
            str >> vertex.z;

            str >> vertex.w;
            if (str.fail())
            {
                str.clear();
                vertex.w = 1.0f;
            }
            Vertices.push_back(vertex);
        }
        else if (ch == "vt")
        {
            Vec2f texCoord;
            str >> texCoord.x;
            str >> texCoord.y;
            TextureCoords.push_back(texCoord);
        }
        else if (ch == "vn")
        {
            Vec3f normals;
            str >> normals.x;
            str >> normals.y;
            str >> normals.z;
            // Do nothing with them
        }
        else if (ch == "f")
        {
            // This is the harder case
            Face     face;
            uint32_t n;
            char     ch;
            while (true)
            {
                str >> ch;
                if (std::isdigit(ch) && !str.eof())
                {
                    str.putback(ch);
                    str >> n;
                    face.vertices.push_back(Vertices.at(n - 1));
                    str >> ch;
                    if (ch == '/')
                    {
                        str >> ch;
                        if (ch == '/')
                        {
                            str >> n; // Read the vertex normal
                        }
                        else
                        {

                            str.putback(ch);
                            str >> n; // Read the texture co-ordinate
                            face.texCoord.push_back(TextureCoords.at(n - 1));
                        }
                        str >> ch;
                        if (ch == '/')
                        {
                            str >> n; // Read the vertex normal now
                        }
                        else
                            str.putback(ch);
                    }
                    else
                        str.putback(ch);
                }
                else
                {
                    if (!str.eof())
                        str.putback(ch);
                    break;
                }
            }
            Faces.push_back(face);
        }
        str >> ch;
    }
    // Load all the materials
    this->ParseMaterials(current_path);
}

void Object3D::ParseMaterials(std::string_view current_path)
{
    for (auto &mat : Materials)
    {
        // open the mtl file of the same name
        std::stringstream str;
        std::ifstream     mat_file(std::string(current_path) + mat.material_path, std::ios::binary | std::ios::in);
        if (!mat_file.is_open())
        {
            std::cerr << std::format("Failed to open material file {}\n", mat.material_path);
        }
        else
        {
            str << mat_file.rdbuf();
            // Do other things here
            std::cerr << "Successfully opened " << mat.material_path << std::endl;
            // Parse the mtl file
            std::string readStr;
            str >> readStr;
            while (!str.eof())
            {
                if (readStr == "newmtl")
                {
                    str >> readStr;
                    mat.material_name = std::move(readStr);
                }
                else if (readStr == "Ns")
                {
                    str >> mat.shiny;
                }
                else if (readStr == "Ks")
                {
                    str >> mat.specular.x;
                    str >> mat.specular.y;
                    str >> mat.specular.z;
                }
                else if (readStr == "Kd")
                {
                    str >> mat.diffuse.x;
                    str >> mat.diffuse.y;
                    str >> mat.diffuse.z;
                }
                else if (readStr == "Ka")
                {
                    str >> mat.ambient.x;
                    str >> mat.ambient.y;
                    str >> mat.ambient.z;
                }
                else if (readStr == "Ni")
                {
                    str >> mat.opticalDensity;
                }
                else if (readStr == "d")
                {
                    str >> mat.dissolve;
                }
                else if (readStr == "illum")
                {
                    // do nothing for now
                }
                else if (readStr == "map_Kd")
                {
                    std::string path;
                    str >> path;
                    mat.texture_id = CreateTexture((std::string(current_path) + path).c_str());
                    std::cout << "Trying to load " << std::string(current_path) + path << "  " << Materials.size()
                              << std::endl;
                }
                str >> readStr;
            }
        }
    }
}

void Object3D::LoadGeometry(std::vector<Pipeline3D::VertexAttrib3D> &vertexList, std::vector<uint32_t> &indexList)
{
    // Now first triangulate the face
    uint32_t                   index = 0;
    for (auto const &face : Faces)
    {
        index    = vertexList.size();
        size_t i = 0;
        // Not sure if the object even have the material .. so need to consider for that first
        for (auto const &vertex : face.vertices)
            vertexList.push_back(Pipeline3D::VertexAttrib3D{.TexCoord = face.texCoord.at(i++), .Position = vertex});
        // vertexList.push_back(Pipeline3D::VertexAttrib3D{.Position = vertex});

        for (uint32_t i = 1; i < face.vertices.size() - 1; ++i)
        {
            indexList.push_back(index);
            indexList.push_back(index + i);
            indexList.push_back(index + i + 1);
        }
    }
}
