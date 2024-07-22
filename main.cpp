#include <iostream>
#include <chrono>
#include "Renderer.h"

int main(int argc, char* argv[]) 
{
    system(".\\CompileShaders");

    Renderer renderer = Renderer("MyFirstVulkanApp", VK_MAKE_API_VERSION(0, 1, 3, 0), "MyEngine", VK_MAKE_API_VERSION(0, 1, 3, 0), int(1280), int(720));

    //double fpsMoy;
    //double alpha = 0.005;
    
    //int frameCountBeforeFpsAverage = 0;

    while (!renderer.WindowShouldClose())
    {
        //auto start = std::chrono::high_resolution_clock::now();

        renderer.Draw();

        /*
        auto end = std::chrono::high_resolution_clock::now();

        double s = std::chrono::duration<double>(end - start).count();
        double fps = 1. / s;

        if (frameCountBeforeFpsAverage < 100)
        {
            frameCountBeforeFpsAverage++;
            fpsMoy = fps;
        }
        else
        {
            fpsMoy = fps * alpha + (1 - alpha) * fpsMoy;
            //std::cout << "Average fps: " << fpsMoy << "\nms: " << s * 1000. << std::endl;
        }
        */
    }

	return 0;
}