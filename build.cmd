@echo off
setlocal
cd /d "%~dp0"
where cl >nul 2>&1
if not errorlevel 1 goto compiler_ready
set "FPV_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%FPV_VSWHERE%" goto missing_compiler
for /f "usebackq tokens=*" %%i in (`"%FPV_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "FPV_VS_ROOT=%%i"
if not defined FPV_VS_ROOT goto missing_compiler
call "%FPV_VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
:compiler_ready
set "FPV_BUILD_OUT=%~1"
if "%FPV_BUILD_OUT%"=="" set "FPV_BUILD_OUT=bin"
if not exist "%FPV_BUILD_OUT%" mkdir "%FPV_BUILD_OUT%"
cl /nologo /std:c++17 /EHsc /O2 /W4 /MT src\tests.cpp /Fe:"%FPV_BUILD_OUT%\FlightTests.exe" /Fo:"%FPV_BUILD_OUT%\tests.obj"
if errorlevel 1 exit /b 1
"%FPV_BUILD_OUT%\FlightTests.exe"
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /W4 /MT /LD src\mod.cpp /Fe:"%FPV_BUILD_OUT%\JC3FPV.dll" /Fo:"%FPV_BUILD_OUT%\mod.obj" /link user32.lib xinput.lib comctl32.lib gdi32.lib
if errorlevel 1 exit /b 1
cl /nologo /std:c++17 /EHsc /O2 /W4 /MT src\launcher.cpp /Fe:"%FPV_BUILD_OUT%\JC3FPV.exe" /Fo:"%FPV_BUILD_OUT%\launcher.obj" /link user32.lib shell32.lib
exit /b %errorlevel%
:missing_compiler
echo Install Visual Studio Build Tools with Desktop development with C++ and a Windows SDK, then run build.cmd again.
exit /b 1
