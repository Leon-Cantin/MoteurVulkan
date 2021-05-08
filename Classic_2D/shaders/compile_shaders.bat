CALL :Compile triangle.vert
CALL :Compile triangle.frag
CALL :Compile text.vert
CALL :Compile text.frag
CALL :Compile copy.vert
CALL :Compile copy.frag

pause
EXIT /B %ERRORLEVEL%

:Compile
@echo off
D:\Programme\VulkanSDK\1.2.148.1\Bin\glslc.exe --target-env=vulkan1.1 .\%~1 -o ..\..\Classic_2D_output\shaders\%~1.spv
@echo on