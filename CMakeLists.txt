# Build on Windows with Visual Studio:  cmake -B build -A x64 && cmake --build build --config Release
cmake_minimum_required(VERSION 3.15)
project(JsonStringConverter CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_library(JsonStringConverter SHARED src/Plugin.cpp src/JsonCore.cpp src/version.rc)
target_compile_definitions(JsonStringConverter PRIVATE UNICODE _UNICODE)
