@echo off
setlocal enabledelayedexpansion

REM Default values
set "CMAKE_PRESET=release"
set "RUN_ENGINE=true"

REM Get the root directory (current dir)
set "ROOT_DIR=%cd%"
set "BUILD_DIR=%ROOT_DIR%\build"

REM Parse arguments
:parse_args
for %%A in (%*) do (
    if "%%A"=="-d" (
        set "CMAKE_PRESET=debug"
    ) else if "%%A"=="-c" (
        set "RUN_ENGINE=false"
    )
)

REM Create build directory
echo Preparing build directory...
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

REM Execute CMake
echo Executing CMake with preset: %CMAKE_PRESET%
pushd "%BUILD_DIR%"
cmake --preset %CMAKE_PRESET% "%ROOT_DIR%" >nul
if errorlevel 1 (
    echo [ERROR] CMake failed.
    popd
    exit /b 1
)

REM Build the executable
echo Building the executable...
make >nul
if errorlevel 1 (
    echo [ERROR] Build failed.
    popd
    exit /b 1
)
popd

REM Run the engine
if "%RUN_ENGINE%"=="true" (
    echo Running the engine...
    "%BUILD_DIR%\chessengine.out"
) else (
    echo Build completed. You can run the engine with build\chessengine.out
)

endlocal
