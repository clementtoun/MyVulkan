#include "GlfwWindow.h"

GlfwWindow::GlfwWindow()
{
    m_window = NULL;

    glfwInit();
}

const char** GlfwWindow::getRequiredInstanceExtesions(uint32_t& extensionsCount)
{
    const char**  extensionsNames = glfwGetRequiredInstanceExtensions(&extensionsCount);

    return extensionsNames;
}

bool GlfwWindow::createWindow(const std::string& WindowName, int width, int height)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(width, height, WindowName.c_str(), NULL, NULL);

    return m_window != NULL;
}

VkResult GlfwWindow::createSurface(VkInstance& instance, VkSurfaceKHR& vkSurface)
{
    return glfwCreateWindowSurface(instance, m_window, NULL, &vkSurface);
}

GLFWwindow* GlfwWindow::getWindow()
{
    return m_window;
}

int GlfwWindow::getWidth()
{
    int width;
    glfwGetWindowSize(m_window, &width, NULL);
    return width;
}

int GlfwWindow::getHeight()
{
    int height;
    glfwGetWindowSize(m_window, NULL, &height);
    return height;
}

void GlfwWindow::destroyWindow()
{
    glfwDestroyWindow(m_window);
}

void GlfwWindow::terminateGlfw()
{
    glfwTerminate();
}
