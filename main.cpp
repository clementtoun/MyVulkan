#include <iostream>
#include <chrono>
#include "Renderer.h"
#include <time.h>

int main(int argc, char* argv[]) 
{
    Renderer renderer = Renderer("MyFirstApp", VK_MAKE_API_VERSION(0, 1, 3, 0), "MyEngine", VK_MAKE_API_VERSION(0, 1, 3, 0), 1280*1.5, 720*1.5);

    double fpsMoy;
    double alpha = 0.00005;
    
    int frameCountBeforeFpsAverage = 0;

    while (!renderer.WindowShouldClose())
    {
        auto start = std::chrono::high_resolution_clock::now();

        renderer.Draw();

        auto end = std::chrono::high_resolution_clock::now();

        double fps = 1. / std::chrono::duration<double>(end - start).count();

        if (frameCountBeforeFpsAverage < 100)
        {
            frameCountBeforeFpsAverage++;
            fpsMoy = fps;
        }
        else
        {
            fpsMoy = fps * alpha + (1 - alpha) * fpsMoy;
            std::cout << "Average fps: " << fpsMoy << std::endl;
        }
    }

	return 0;
}