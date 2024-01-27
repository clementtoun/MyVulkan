#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "MeshLoader.h"

using namespace tinyobj;

Mesh* MeshLoader::loadMesh(const std::string& path, const std::string& mtlSearchPath)
{
	ObjReaderConfig readerConfig;
    readerConfig.vertex_color = false;
    readerConfig.triangulate = true;
    readerConfig.triangulation_method = "earcut";

    if (mtlSearchPath.empty())
        readerConfig.mtl_search_path = path.substr(0, path.find_last_of('/') + 1);
    else
        readerConfig.mtl_search_path = mtlSearchPath;

	ObjReader reader;

    if (!reader.ParseFromFile(path, readerConfig))
    {
        if (!reader.Error().empty())
        {
            std::cout << "TinyObjReader: " << reader.Error();
        }
        return nullptr;
    }

	if (!reader.Warning().empty())
		std::cout << "TinyObjReader: " << reader.Warning();

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::set<uint32_t> verticesToSetNormal;

    for (size_t i = 0; i < attrib.vertices.size(); i+=3)
    {
        Vertex currentVertex{};
        currentVertex.pos.x = attrib.vertices[i];
        currentVertex.pos.y = attrib.vertices[i+1];
        currentVertex.pos.z = attrib.vertices[i+2];

        currentVertex.color[0] = 255 * 0.5;
        currentVertex.color[1] = 255 * 0.5;
        currentVertex.color[2] = 255 * 0.5;

        verticesToSetNormal.insert(i/3);
        vertices.emplace_back(currentVertex);
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

            std::array<index_t, 3> idxs = { shapes[s].mesh.indices[index_offset],
                                            shapes[s].mesh.indices[index_offset + 1],
                                            shapes[s].mesh.indices[index_offset + 2] };

            for (const auto& idx : idxs)
            {
                indices.emplace_back(idx.vertex_index);

                if (verticesToSetNormal.contains(idx.vertex_index))
                {
                    vertices[idx.vertex_index].normal.x = attrib.normals[3 * idx.normal_index];
                    vertices[idx.vertex_index].normal.y = attrib.normals[3 * idx.normal_index + 1];
                    vertices[idx.vertex_index].normal.z = attrib.normals[3 * idx.normal_index + 2];

                    verticesToSetNormal.erase(idx.vertex_index);
                }
            }

            index_offset += size_t(shapes[s].mesh.num_face_vertices[f]);
        }
    }

	return new Mesh(vertices, indices);
}
