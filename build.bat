@echo off
setlocal
rem Build Lida - requires Visual Studio 2022 with the C++ desktop workload (or Build Tools).
set "VSDIR="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
if defined VSDIR if exist "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul
if not defined VSDIR if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if not defined VSDIR if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
where cl >nul 2>&1 || (echo Could not find the Visual C++ compiler. Install "Desktop development with C++". & exit /b 1)
cd /d "%~dp0"
if not exist build mkdir build
rc /nologo /fo build\resource.res resource.rc || exit /b 1
cl /nologo /std:c++17 /EHsc /W3 /O2 /DUNICODE /D_UNICODE /utf-8 /Fo:build\ /Fd:build\ src\main.cpp build\resource.res ^
   user32.lib gdi32.lib shell32.lib advapi32.lib comdlg32.lib ole32.lib ^
   /link /SUBSYSTEM:WINDOWS /OUT:Lida.exe || exit /b 1
echo Built Lida.exe
