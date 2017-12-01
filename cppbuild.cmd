@echo off

set CPPBUILD_VERSION=0.1
set CPPBUILD_DIRECTORY=%~dp0
set WORKING_DIRECTORY=%cd%

if "%1"=="--version" (
    echo cppbuild %CPPBUILD_VERSION%
    echo Copyright 2016 Wouter Saaltink
    echo.
    goto :exit
)

if not exist %CPPBUILD_DIRECTORY%\cppbuild.hpp (
    echo.
    echo ***ERROR*** CPPBuild is not installed properly because cppbuild.hpp is missing. It should be next to cppbuild.bat
    goto :exit
)

if not exist %WORKING_DIRECTORY%\__build__.cpp (
    echo.
    echo ***ERROR*** CPPBuild is cannot find the project build file __build__.cpp
    goto :exit
)

if not exist build mkdir build
if not exist build\obj mkdir build\obj

echo Start generating build scripts for this project...
g++ "__build__.cpp" --std=c++11 -o "build\cppbuild.exe" -I%CPPBUILD_DIRECTORY%

call build\cppbuild.exe build > build\build.cmd
call build\cppbuild.exe install > build\install.cmd
call build\cppbuild.exe run > build\run.cmd
call build\cppbuild.exe clean > build\clean.cmd

echo done.
echo.

if "%1"=="" (
	echo Start building this project...
	call build\build.cmd
	echo done.
	echo.
)
if "%1"=="build" (
	echo Start building this project...
	call build\build.cmd
	echo done.
	echo.
)
if "%1"=="install" (
	echo Start installing this project...
	call build\install.cmd
	echo done.
	echo.
)
if "%1"=="run" (
	echo Start running this project...
	call build\run.cmd
	echo done.
	echo.
)
if "%1"=="clean" (
	echo Start cleaning this project...
	call build\clean.cmd
	echo done.
	echo.
)

:exit
