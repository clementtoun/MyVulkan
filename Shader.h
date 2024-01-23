#pragma once

#include "VulkanBase.h"
#include <iostream>
#include <string>
#include <vector>

class Shader
{
public:
	Shader();

	void createModule(VkDevice device, const std::string& path);

	void cleanup(VkDevice device);

	VkShaderModule getShaderModule();

private:
    std::string fileToString(const std::string& path);

	VkShaderModule m_ShaderModule;
};

