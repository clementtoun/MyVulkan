#define TINYGLTF_USE_CPP14
#define TINYGLTF_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT

#include <map>
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

        currentVertex.tex_coord = glm::vec2(0., 0.);

        currentVertex.color[0] = 126;
        currentVertex.color[1] = 126;
        currentVertex.color[2] = 126;

        if (attrib.normals.size() > 0)
            verticesToSetNormal.insert(uint32_t(i/3));
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


Mesh* loadMeshGltf(tinygltf::Model &model, tinygltf::Mesh &mesh, glm::mat4 transform, const std::map<int, size_t>& mapMaterialId, bool autoComputeNormal, bool autoComputeTangent)
{
    Mesh* InternalMesh = new Mesh();

    for (size_t i = 0; i < mesh.primitives.size(); i++)
    {
        tinygltf::Primitive& primitive = mesh.primitives[i];
        tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];

        tinygltf::Accessor positionAccessor{};
        tinygltf::Accessor normalAccessor{};
        tinygltf::Accessor tangentAccessor{};
        tinygltf::Accessor texCoordAccessor{};

        bool NormalFromFile = false;
        bool TangentFromFile = false;

        for (auto& attrib : primitive.attributes) 
        {
            if (attrib.first.compare("POSITION") == 0)
                positionAccessor = model.accessors[attrib.second];

            if (attrib.first.compare("NORMAL") == 0)
            {
                normalAccessor = model.accessors[attrib.second];
                NormalFromFile = true;
            }

            if (attrib.first.compare("TANGENT") == 0)
            {
                tangentAccessor = model.accessors[attrib.second];
                TangentFromFile = true;
            }

            if (attrib.first.compare("TEXCOORD_0") == 0)
                texCoordAccessor = model.accessors[attrib.second];
        }

        NormalFromFile = NormalFromFile && !autoComputeNormal;
        TangentFromFile = TangentFromFile && !autoComputeTangent;

        if (positionAccessor.bufferView == -1 || texCoordAccessor.bufferView == -1)
            return nullptr;

        size_t nbVertexs = positionAccessor.count;

        tinygltf::BufferView& positionBufferView = model.bufferViews[positionAccessor.bufferView];
        tinygltf::BufferView& texCoordBufferView = model.bufferViews[texCoordAccessor.bufferView];

        tinygltf::BufferView normalBufferView{};
        const float* normals = nullptr;
        int normalIdxStride = -1;
        if (NormalFromFile)
        {
            normalBufferView = model.bufferViews[normalAccessor.bufferView];
            normals = reinterpret_cast<const float*>(&(model.buffers[normalBufferView.buffer].data[normalAccessor.byteOffset + normalBufferView.byteOffset]));
            normalIdxStride = normalAccessor.ByteStride(normalBufferView) / sizeof(float);
        }

        tinygltf::BufferView tangentBufferView{};
        const float* tangents = nullptr;
        int tangentIdxStride = -1;
        if (TangentFromFile)
        {
            tangentBufferView = model.bufferViews[tangentAccessor.bufferView];
            tangents = reinterpret_cast<const float*>(&(model.buffers[tangentBufferView.buffer].data[tangentAccessor.byteOffset + tangentBufferView.byteOffset]));
            tangentIdxStride = tangentAccessor.ByteStride(tangentBufferView) / sizeof(float);
        }

        const float* positions = reinterpret_cast<const float*>(&(model.buffers[positionBufferView.buffer].data[positionAccessor.byteOffset + positionBufferView.byteOffset]));
        const float* texCoords = reinterpret_cast<const float*>(&(model.buffers[texCoordBufferView.buffer].data[texCoordAccessor.byteOffset + texCoordBufferView.byteOffset]));

        int positionIdxStride = positionAccessor.ByteStride(positionBufferView) / sizeof(float);
        int texCoordIdxStride = texCoordAccessor.ByteStride(texCoordBufferView) / sizeof(float);

        uint32_t vertexOffset = uint32_t(InternalMesh->GetVertex().size());

        glm::mat3 vectorTransform = glm::inverseTranspose(glm::mat3(transform));
        
        for (size_t j = 0; j < nbVertexs; j++)
        {
            glm::vec3 position;
            position.x = positions[j * positionIdxStride];
            position.y = positions[j * positionIdxStride + 1];
            position.z = positions[j * positionIdxStride + 2];

            position = glm::vec3(transform * glm::vec4(position, 1.f));

            glm::vec3 normal = glm::vec3(0.);
            if (NormalFromFile)
            {
                normal.x = normals[j * normalIdxStride];
                normal.y = normals[j * normalIdxStride + 1];
                normal.z = normals[j * normalIdxStride + 2];
                normal = glm::normalize(vectorTransform * normal);
            }

            glm::vec3 tangent = glm::vec3(0.);
            if (TangentFromFile)
            {
                tangent.x = tangents[j * tangentIdxStride];
                tangent.y = tangents[j * tangentIdxStride + 1];
                tangent.z = tangents[j * tangentIdxStride + 2];
                tangent = glm::normalize(vectorTransform * tangent);
            }

            glm::vec2 texCoord;
            texCoord.x = texCoords[j * texCoordIdxStride];
            texCoord.y = texCoords[j * texCoordIdxStride + 1];

            InternalMesh->AddVertex(Vertex{ position, normal, tangent, glm::vec3(0.f), texCoord, {126, 126, 126} });
        }

        size_t nbIndices = indexAccessor.count;
        uint32_t indexOffset = uint32_t(InternalMesh->GetIndexes().size());

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

        Primitive InternalPrimitive{};
        InternalPrimitive.firstIndex = indexOffset;
        InternalPrimitive.indexCount = static_cast<uint32_t>(nbIndices);
        InternalPrimitive.vertexOffset = vertexOffset;
        InternalPrimitive.vertexCount = static_cast<uint32_t>(InternalMesh->GetVertex().size()) - vertexOffset;
        InternalPrimitive.materialID = mapMaterialId.at(primitive.material);

        size_t primitiveIndex = InternalMesh->AddPrimitives(InternalPrimitive);

        if (!NormalFromFile)
            InternalMesh->AutoComputeNormalsPrimitive(primitiveIndex);
        if (!TangentFromFile)
            InternalMesh->AutoComputeTangentsBiTangentsPrimitive(primitiveIndex);
        else
            InternalMesh->AutoComputeBiTangentsPrimitive(primitiveIndex);
    }

    return InternalMesh;
}

void loadModelMaterials(Materials& materials, tinygltf::Model& model, std::string basePath, std::map<int, size_t>& mapMaterialId)
{
    for (int i = 0; i < model.materials.size(); i++)
    {
        tinygltf::Material& material = model.materials[i];
        tinygltf::PbrMetallicRoughness& pbrMetallicRoughness = material.pbrMetallicRoughness;

        glm::vec3 baseColor = glm::vec3(pbrMetallicRoughness.baseColorFactor[0], pbrMetallicRoughness.baseColorFactor[1], pbrMetallicRoughness.baseColorFactor[2]);
        float metallic = float(pbrMetallicRoughness.metallicFactor);
        float roughness = float(pbrMetallicRoughness.roughnessFactor);

        int baseColorTextureIndex = pbrMetallicRoughness.baseColorTexture.index;
        std::string baseColorTexturePath = "";
        if (baseColorTextureIndex != -1)
            baseColorTexturePath = basePath + "/" + model.images[model.textures[baseColorTextureIndex].source].uri;

        int metallicRoughnessTextureIndex = pbrMetallicRoughness.metallicRoughnessTexture.index;
        std::string metallicRoughnessTexturePath = "";
        if (metallicRoughnessTextureIndex != -1)
            metallicRoughnessTexturePath = basePath + "/" + model.images[model.textures[metallicRoughnessTextureIndex].source].uri;

        int normalTextureIndex = material.normalTexture.index;
        std::string normalTexturePath = "";
        if (normalTextureIndex != -1)
            normalTexturePath = basePath + "/" + model.images[model.textures[normalTextureIndex].source].uri;

        int emissiveTextureIndex = material.emissiveTexture.index;
        std::string emissiveTexturePath = "";
        if (emissiveTextureIndex != -1)
            emissiveTexturePath = basePath + "/" + model.images[model.textures[emissiveTextureIndex].source].uri;

        int AOTextureIndex = material.occlusionTexture.index;
        std::string AOTexturePath = "";
        if (AOTextureIndex != -1)
            AOTexturePath = basePath + "/" + model.images[model.textures[AOTextureIndex].source].uri;

        Material myMaterial;
        myMaterial.materialUniformBuffer.baseColor = baseColor;
        myMaterial.materialUniformBuffer.metallic = metallic;
        myMaterial.materialUniformBuffer.roughness = roughness;
        myMaterial.baseColorTexturePath = baseColorTexturePath;
        myMaterial.metallicRoughnessTexturePath = metallicRoughnessTexturePath;
        myMaterial.normalTexturePath = normalTexturePath;
        myMaterial.emissiveTexturePath = emissiveTexturePath;
        myMaterial.AOTexturePath = AOTexturePath;
        myMaterial.baseColorTexture = nullptr;
        myMaterial.metallicRoughnessTexture = nullptr;
        myMaterial.normalTexture = nullptr;
        myMaterial.emissiveTexture = nullptr;
        myMaterial.AOTexture = nullptr;
        myMaterial.materialUniformBuffer.useBaseColorTexture = false;
        myMaterial.materialUniformBuffer.useMetallicRoughnessTexture = false;
        myMaterial.materialUniformBuffer.useNormalTexture = false;
        myMaterial.materialUniformBuffer.useEmissiveTexture = false;
        myMaterial.materialUniformBuffer.useAOTexture = false;

        size_t materialIndex = materials.AddMaterial(std::move(myMaterial));

        mapMaterialId[i] = materialIndex;
    }
}

void loadModelNodes(tinygltf::Model& model, tinygltf::Node& node, glm::mat4 parentTransform, std::vector<Mesh*>& meshes, const std::map<int, size_t>& mapMaterialId, bool autoComputeNormal, bool autoComputeTangent)
{
    glm::mat4 nodeTransform = glm::mat4(1.f);

    if (node.matrix.size() > 0)
    {
        nodeTransform = glm::mat4(node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
                                  node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
                                  node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11], 
                                  node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
    }
    else
    {
        glm::vec3 translation = glm::vec3(0.f);
        glm::quat rotation{};
        rotation.x = 0.f;
        rotation.y = 0.f;
        rotation.z = 0.f;
        rotation.w = 1.f;
        glm::vec3 scale = glm::vec3(1.f);

        if (node.translation.size() > 0)
            translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);

        if (node.rotation.size() > 0)
            rotation = glm::make_quat(node.rotation.data());

        if (node.scale.size() > 0)
            scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);

        nodeTransform = glm::translate(nodeTransform, translation);
        nodeTransform = nodeTransform * glm::mat4_cast(rotation);
        nodeTransform = glm::scale(nodeTransform, scale);
    }

    nodeTransform = parentTransform * nodeTransform;

    if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
    {
        Mesh* meshInternal = loadMeshGltf(model, model.meshes[node.mesh], nodeTransform, mapMaterialId, autoComputeNormal, autoComputeTangent);
        if (meshInternal)
            meshes.push_back(meshInternal);
    }

    for (size_t i = 0; i < node.children.size(); i++)
    {
        loadModelNodes(model, model.nodes[node.children[i]], nodeTransform, meshes, mapMaterialId, autoComputeNormal, autoComputeTangent);
    }
}

std::vector<Mesh*> MeshLoader::loadGltf(const std::string& path, Materials& materials, bool autoComputeNormal, bool autoComputeTangent)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err;
    std::string warn;

    std::string extension = path.substr(path.find_last_of('.'));

    bool res = false;
    if (extension._Equal(".glb"))
        res = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    else
        res = loader.LoadASCIIFromFile(&model, &err, &warn, path);
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

    size_t lastPathSepIndex = path.find_last_of('/');
    std::map<int, size_t> mapMaterialId;

    loadModelMaterials(materials, model, path.substr(0, lastPathSepIndex), mapMaterialId);

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];
    for (size_t i = 0; i < scene.nodes.size(); i++)
    {
        loadModelNodes(model, model.nodes[scene.nodes[i]], glm::mat4(1.), meshes, mapMaterialId, autoComputeNormal, autoComputeTangent);
    }

    return meshes;
}
