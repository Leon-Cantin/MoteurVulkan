CALL :Compile pbr_opaque.vert
CALL :Compile pbr_opaque.frag
CALL :Compile skybox.vert
CALL :Compile skybox.frag
CALL :Compile textCompute.comp
CALL :Compile text.vert
CALL :Compile text.frag
CALL :Compile shadows.vert
CALL :Compile shadows.frag

pause
EXIT /B %ERRORLEVEL%

:Compile
@echo off
D:\Programme\VulkanSDK\1.2.148.1\Bin\glslc.exe --target-env=vulkan1.1 .\%~1 -o ..\..\output\shaders\%~1.spv
@echo on