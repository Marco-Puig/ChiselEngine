cmake_minimum_required(VERSION 3.24)
project(ChiselEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Put binaries in one predictable place
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(CHISEL_ENABLE_OPENXR "Build OpenXR headset support" ON)

include(FetchContent)

# -----------------------------------------------------
# Dependency options (must be set before MakeAvailable)
# -----------------------------------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# tinygltf: compile tiny_gltf.cc into a real static lib.
# HEADER_ONLY is what causes duplicate-symbol pain, so leave it OFF
# and never #define TINYGLTF_IMPLEMENTATION in your own sources.
set(TINYGLTF_HEADER_ONLY OFF CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_GL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_VALIDATOR_EXAMPLE OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_BUILDER_EXAMPLE OFF CACHE BOOL "" FORCE)

# -----------------------------------------------------
# Dependencies
# -----------------------------------------------------
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
    GIT_SHALLOW    TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.9.3
    GIT_SHALLOW    TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
)

# ImGui ships no CMakeLists, so this only downloads it.
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE
)

if(CHISEL_ENABLE_OPENXR)
    FetchContent_Declare(openxr
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK-Source.git
        GIT_TAG        release-1.1.46
        GIT_SHALLOW    TRUE
        SYSTEM
        EXCLUDE_FROM_ALL
    )
endif()

FetchContent_MakeAvailable(glfw glm glad tinygltf imgui)
if(CHISEL_ENABLE_OPENXR)
    set(DYNAMIC_LOADER OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(openxr)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# -----------------------------------------------------
# Dear ImGui (target name differs from the content name)
# -----------------------------------------------------
add_library(imgui_backend STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui_backend SYSTEM PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Route ImGui's GL calls through glad instead of its bundled imgl3w loader.
target_compile_definitions(imgui_backend PUBLIC IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
target_link_libraries(imgui_backend PUBLIC glad glfw OpenGL::GL)
if(MSVC)
    target_compile_options(imgui_backend PRIVATE /FI"glad/glad.h")
else()
    target_compile_options(imgui_backend PRIVATE -include glad/glad.h)
endif()

# -----------------------------------------------------
# Main Executable
# -----------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(ChiselEngine ${SOURCES})

target_include_directories(ChiselEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(ChiselEngine PRIVATE GLFW_INCLUDE_NONE)
if(CHISEL_ENABLE_OPENXR)
    target_compile_definitions(ChiselEngine PRIVATE
        CHISEL_ENABLE_OPENXR
        XR_USE_GRAPHICS_API_OPENGL
    )
    if(WIN32)
        target_compile_definitions(ChiselEngine PRIVATE XR_USE_PLATFORM_WIN32)
    endif()
    target_link_libraries(ChiselEngine PRIVATE openxr_loader)
endif()

target_link_libraries(ChiselEngine PRIVATE
    glad
    glfw
    glm::glm
    tinygltf
    OpenGL::GL
    imgui_backend
)

if(WIN32)
    target_compile_definitions(ChiselEngine PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
    )
endif()

if(MSVC)
    target_compile_options(ChiselEngine PRIVATE /W4 /permissive-)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                 PROPERTY VS_STARTUP_PROJECT ChiselEngine)
else()
    target_compile_options(ChiselEngine PRIVATE -Wall -Wextra)
endif()

# -----------------------------------------------------
# Post-Build: Copy Resources
# -----------------------------------------------------
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/resources)
    add_custom_command(TARGET ChiselEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/resources
            $<TARGET_FILE_DIR:ChiselEngine>/resources
        COMMENT "Copying Resources next to the executable"
    )
endif()