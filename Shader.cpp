#include "Shader.h"

#include <fstream>
#include <sstream>

Shader::Shader()
{
    m_ShaderModule = VK_NULL_HANDLE;
}

void Shader::createModule(VkDevice device, const std::string& path)
{
    std::string shaderCode = fileToString(path);

    VkShaderModuleCreateInfo vertexShaderCreateInfo;
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderCreateInfo.pNext = NULL;
    vertexShaderCreateInfo.flags = 0;
    vertexShaderCreateInfo.codeSize = shaderCode.size();
    vertexShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    if (vkCreateShaderModule(device, &vertexShaderCreateInfo, NULL, &m_ShaderModule) != VK_SUCCESS)
        std::cout << "Shader module creation failed !" << std::endl;
}

void Shader::cleanup(VkDevice device)
{
    if (m_ShaderModule != VK_NULL_HANDLE)
        vkDestroyShaderModule(device, m_ShaderModule, NULL);
}

VkShaderModule Shader::getShaderModule()
{
    return m_ShaderModule;
}

std::string Shader::fileToString(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);

    std::string fileString = "";

    if (file)
    {
        std::stringstream strStream;
        strStream << file.rdbuf();

        fileString = strStream.str();

        file.close();
    }

    return fileString;
}