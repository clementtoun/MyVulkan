#pragma once

#include "tiny_gltf.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include "Mesh.h"
#include "Materials.h"
#include <glm/gtc/quaternion.hpp>
#include <iostream>

namespace MeshLoader
{
	Mesh* loadMeshObj(const std::string& path, const std::string& mtlSearchPath = "");

	std::vector<Mesh*> loadGltf(const std::string& path, Materials& materials, bool autoComputeNormal = false, bool autoComputeTangent = false);
}

