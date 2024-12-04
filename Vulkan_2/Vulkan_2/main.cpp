#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include "Constants.h"
#include "HelloTriangleApplication.h"

int main() 
{
#pragma region Compile shaders

    std::filesystem::path exePath = std::filesystem::current_path();
    std::filesystem::path batchFilePath = exePath / "shaders\\compile.bat";
    std::string command = batchFilePath.string();
    std::filesystem::current_path(exePath / "shaders\\");
    int result = system(command.c_str());
	if(result != 0)
    {
        throw std::runtime_error("Failed to compile shaders!!!");
        return EXIT_FAILURE;
    }
    // reset the execution path
    std::filesystem::current_path(exePath);

#pragma endregion

    HelloTriangleApplication app;

    try 
    {
        app.run();
    }
    catch (const std::exception& e) 
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}