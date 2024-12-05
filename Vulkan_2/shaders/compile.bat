@echo off

D:\Libraries\VulkanSDK\1.3.296.0\Bin\glslc.exe shader.vert -o vert.spv
if %errorlevel% neq 0 (
	echo Failed to compile shader.vert
	exit /b 1
	)

D:\Libraries\VulkanSDK\1.3.296.0\Bin\glslc.exe shader.frag -o frag.spv
if %errorlevel% neq 0 (
	echo Failed to compile shader.vert
	exit /b 1
	)
	
echo All shaders compiled successfully.
exit /b 0