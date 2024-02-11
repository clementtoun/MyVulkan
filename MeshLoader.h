#pragma once

#include "tiny_gltf.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include "Mesh.h"
#include <iostream>

namespace MeshLoader
{
	Mesh* loadMeshObj(const std::string& path, const std::string& mtlSearchPath = "");

	std::vector<Mesh*> loadGltf(const std::string& path);

}

