cmake_minimum_required(VERSION 3.24)
project(RendererNative LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

# 查找Java头文件
find_package(JNI REQUIRED)
include_directories(${JNI_INCLUDE_DIRS})

# Windows特定设置
if(WIN32)
    include_directories("include")
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

# Vulkan支持（可选）
if(DEFINED ENV{VULKAN_SDK})
    include_directories("$ENV{VULKAN_SDK}/Include")
    link_directories("$ENV{VULKAN_SDK}/Lib")
endif()

add_library(RendererNative SHARED
    src/RendererNative.cpp
    src/OpenGLBackend.cpp
    src/VulkanBackend.cpp
    src/D3D11Backend.cpp
)

# 链接库
if(WIN32)
    target_link_libraries(RendererNative d3d11 d3dcompiler ${JNI_LIBRARIES})
    if(DEFINED ENV{VULKAN_SDK})
        target_link_libraries(RendererNative vulkan-1)
    endif()
endif()

# 输出目录
set_target_properties(RendererNative PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build"
)