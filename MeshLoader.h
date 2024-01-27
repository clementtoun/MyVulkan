#pragma once

#include "tinyobjloader/tiny_obj_loader.h"
#include "Mesh.h"
#include <iostream>

namespace MeshLoader
{
	Mesh* loadMesh(const std::string& path, const std::string& mtlSearchPath = "");
};

