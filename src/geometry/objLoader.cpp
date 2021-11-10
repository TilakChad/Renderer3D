#include "../include/geometry.hpp"
#include "../include/rasteriser.h"

#include <fstream>
#include <sstream>

Object3D::Object3D(std::string_view obj_path)
{
    std::ifstream objfile(obj_path.data(), std::ios::binary | std::ios::in);
    if (!objfile.is_open())
    {
        std::cerr << "Failed to open file " << obj_path << std::endl;
    }
    else
    {
        // objfile.read(&ch, sizeof ch);
        std::stringstream buffer;
        buffer << objfile.rdbuf();
        ParseOBJ(buffer);
    }
}

void Object3D::ParseOBJ(std::stringstream &str)
{
    std::string ch;
    str >> ch;
    while (!str.eof())
    {
        if (ch == "v")
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
}

void Object3D::LoadGeometry(std::vector<Pipeline3D::VertexAttrib3D> &vertexList)
{
    // Now first triangulate the face
    // This is not the way face are triangulated 
    Pipeline3D::VertexAttrib3D vertex;
    for (auto const &face : Faces)
    {
        for (uint32_t i = 1; i < face.vertices.size() - 1; ++i)
        {
            vertex.Position = face.vertices.at(0);
            // vertex.TexCoord = face.texCoord.at(0);
            vertexList.push_back(vertex);

            vertex.Position = face.vertices.at(i);
            // vertex.TexCoord = face.texCoord.at(i);
            vertexList.push_back(vertex);

            vertex.Position = face.vertices.at(i + 1);
            // vertex.TexCoord = face.texCoord.at(i + 1);
            vertexList.push_back(vertex);
        }
    }
}
