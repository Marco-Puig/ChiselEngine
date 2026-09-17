cmake_minimum_required(VERSION 3.14)
project(ChiselEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_DEFAULT_CMP0000 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)

# GLFW
FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.3.8)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 0.9.9.8)
FetchContent_MakeAvailable(glm)

# Dear ImGui
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG v1.89)
FetchContent_MakeAvailable(imgui)

# Assimp (Temporarily disabled to allow engine to boot)
# set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
# set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
# set(ASSIMP_BUILD_EXPORT OFF CACHE BOOL "Disable all exporters to avoid build errors" FORCE)
# FetchContent_Declare(assimp GIT_REPOSITORY https://github.com/assimp/assimp.git GIT_TAG v5.3.1)
# FetchContent_MakeAvailable(assimp)

# Note: GLAD and OpenXR usually require platform-specific installers or generators.
# For this setup, we assume they will be linked or provided via system paths/submodules.
# In a real scenario, we'd use a GLAD generator and the OpenXR SDK installer.

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(ChiselEngine ${SOURCES})

target_link_libraries(ChiselEngine PRIVATE glfw glm)

target_include_directories(ChiselEngine PRIVATE src)

# Copy Resources to build directory
add_custom_command(TARGET ChiselEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Resources"
    "$<TARGET_FILE_DIR:ChiselEngine>/Resources")
