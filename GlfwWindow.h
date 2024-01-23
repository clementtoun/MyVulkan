#pragma once

#include "VulkanBase.h"
#include <iostream>
#include <string>

class GlfwWindow
{
public:

	GlfwWindow();

	const char** getRequiredInstanceExtesions(uint32_t& extensionsCount);

	bool createWindow(const std::string& WindowName, int width = 1080, int height = 720);

	VkResult createSurface(VkInstance& instance, VkSurfaceKHR& vkSurface);

	GLFWwindow* getWindow();

	int getWidth();
	int getHeight();

	void destroyWindow();

	void terminateGlfw();

private:
	GLFWwindow* m_window;
};

