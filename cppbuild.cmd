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
call build\cppbuild.exe test > build\test.cmd

echo done.
echo.

if "%1"=="" (
    echo Start building this project...
    echo.
    call build\build.cmd
    echo done.
    echo.
)
if "%1"=="build" (
    echo Start building this project...
    echo.
    call build\build.cmd %2
    echo done.
    echo.
)
if "%1"=="install" (
    echo Start installing this project...
    echo.
    call build\install.cmd %2
    echo done.
    echo.
)
if "%1"=="run" (
    if "%2"=="" (
        echo No target given to run this project.
    ) else (
        echo Start running this project...
        echo.
        call build\run.cmd %2
        echo done.
        echo.
    )
)
if "%1"=="clean" (
    echo Start cleaning this project...
    echo.
    call build\clean.cmd %2
    echo done.
    echo.
)
if "%1"=="test" (
    echo Start testing this project...
    echo.
    call build\test.cmd %2
    echo done.
    echo.
)

:exit
