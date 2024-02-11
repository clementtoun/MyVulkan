#define TINYGLTF_USE_CPP14
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "MeshLoader.h"

Mesh* MeshLoader::loadMeshObj(const std::string& path, const std::string& mtlSearchPath)
{
    tinyobj::ObjReaderConfig readerConfig;
    readerConfig.vertex_color = false;
    readerConfig.triangulate = true;
    readerConfig.triangulation_method = "earcut";

    if (mtlSearchPath.empty())
        readerConfig.mtl_search_path = path.substr(0, path.find_last_of('/') + 1);
    else
        readerConfig.mtl_search_path = mtlSearchPath;

    tinyobj::ObjReader reader;

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

        currentVertex.normal = glm::vec3(0.);

        currentVertex.color[0] = 255 * 0.5;
        currentVertex.color[1] = 255 * 0.5;
        currentVertex.color[2] = 255 * 0.5;

        if (attrib.normals.size() > 0)
            verticesToSetNormal.insert(i/3);
        vertices.emplace_back(currentVertex);
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

            std::array<tinyobj::index_t, 3> idxs = { shapes[s].mesh.indices[index_offset],
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

    auto mesh = new Mesh(vertices, indices);

    if (attrib.normals.size() == 0)
        mesh->AutoComputeNormals();

	return mesh;
}

template <class T>
void getIndices(const unsigned char* bufferData, size_t nbIndices, Mesh* mesh)
{
    const T* indexes = reinterpret_cast<const T*>(bufferData);

    for (size_t i = 0; i < nbIndices; i++)
        mesh->AddIndex(uint32_t(indexes[i]));
}


Mesh* loadMeshGltf(tinygltf::Model &model, tinygltf::Mesh &mesh)
{
    Mesh* InternalMesh = new Mesh();

    for (size_t i = 0; i < mesh.primitives.size(); i++)
    {

        tinygltf::Primitive& primitive = mesh.primitives[i];
        tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

        tinygltf::Accessor positionAccessor{};
        tinygltf::Accessor normalAccessor{};
        for (auto& attrib : primitive.attributes) 
        {
            if (attrib.first.compare("POSITION") == 0)
                positionAccessor = model.accessors[attrib.second];
            if (attrib.first.compare("NORMAL") == 0)
                normalAccessor = model.accessors[attrib.second];
        }

        if (positionAccessor.bufferView == -1 || normalAccessor.bufferView == -1)
            return nullptr;

        size_t nbVertexs = positionAccessor.count;

        tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
        tinygltf::BufferView& normalBufferView = model.bufferViews[normalAccessor.bufferView];

        const float* positions = reinterpret_cast<const float*>(&model.buffers[positionBufferView.buffer].data[positionAccessor.byteOffset + positionBufferView.byteOffset]);
        const float* normals = reinterpret_cast<const float*>(&model.buffers[normalBufferView.buffer].data[normalAccessor.byteOffset + normalBufferView.byteOffset]);

        int positionIdxStride = positionAccessor.ByteStride(positionBufferView) / sizeof(float);
        int normalIdxStride = normalAccessor.ByteStride(normalBufferView) / sizeof(float);

        uint32_t vertexOffset = InternalMesh->GetVertex().size();
        
        for (size_t j = 0; j < nbVertexs; j++)
        {
            glm::vec3 position;
            position.x = positions[j * positionIdxStride];
            position.y = positions[j * positionIdxStride + 1];
            position.z = positions[j * positionIdxStride + 2];

            glm::vec3 normal;
            normal.x = normals[j * normalIdxStride];
            normal.y = normals[j * normalIdxStride + 1];
            normal.z = normals[j * normalIdxStride + 2];
            InternalMesh->AddVertex(Vertex{ position, normal, {127, 127, 127}});
        }

        size_t nbIndices = indexAccessor.count;
        uint32_t indexOffset = InternalMesh->GetIndexes().size();

        tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const unsigned char* indexBufferData = &model.buffers[indexBufferView.buffer].data[indexAccessor.byteOffset + indexBufferView.byteOffset];

        switch (indexAccessor.componentType)
        {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
            {
                getIndices<unsigned int>(indexBufferData, nbIndices, InternalMesh);
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
            {
                getIndices<unsigned short>(indexBufferData, nbIndices, InternalMesh);
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
            {
                getIndices<unsigned char>(indexBufferData, nbIndices, InternalMesh);
                break;
            }
            default:
            {
                return nullptr;
                break;
            }
        }

        Primitive InternalPrimitive;
        InternalPrimitive.firstIndex = indexOffset;
        InternalPrimitive.indexCount = static_cast<uint32_t>(nbIndices);
        InternalPrimitive.vertexOffset = vertexOffset;
        InternalPrimitive.materialID = primitive.material;

        InternalMesh->AddPrimitives(InternalPrimitive);
    }

    return InternalMesh;
}

void loadModelNodes(tinygltf::Model& model, tinygltf::Node &node, std::vector<Mesh*>& meshes)
{
    if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
    {
        Mesh* meshInternal = loadMeshGltf(model, model.meshes[node.mesh]);
        if (meshInternal)
            meshes.push_back(meshInternal);
    }

    for (size_t i = 0; i < node.children.size(); i++)
    {
        loadModelNodes(model, model.nodes[node.children[i]], meshes);
    }
}

std::vector<Mesh*> MeshLoader::loadGltf(const std::string& path)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cout << "ERR: " << err << std::endl;
    }

    if (!res)
        std::cout << "Failed to load glTF: " << path << std::endl;
    else
        std::cout << "Loaded glTF: " << path << std::endl;

    std::vector<Mesh*> meshes;

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        loadModelNodes(model, model.nodes[scene.nodes[i]], meshes);
    }

    return meshes;
}
