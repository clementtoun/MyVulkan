.\glslc.exe .\Shader\firstShader.vert -o .\Shader\firstVert.spv
.\glslc.exe .\Shader\firstShader.frag -o .\Shader\firstFrag.spv

.\glslc.exe .\Shader\secondShader.vert -o .\Shader\secondVert.spv
.\glslc.exe .\Shader\secondShader.frag -o .\Shader\secondFrag.spv

.\glslc.exe .\Shader\cubeMapShader.vert -o .\Shader\cubeMapVert.spv
.\glslc.exe .\Shader\cubeMapShader.frag -o .\Shader\cubeMapFrag.spv

.\glslangValidator.exe -V --target-env vulkan1.2 .\Shader\raygen.rgen -o .\Shader\raygen.spv
.\glslangValidator.exe -V --target-env vulkan1.2 .\Shader\miss.rmiss -o .\Shader\miss.spv
.\glslangValidator.exe -V --target-env vulkan1.2 .\Shader\closesthit.rchit -o .\Shader\closesthit.spv

pause