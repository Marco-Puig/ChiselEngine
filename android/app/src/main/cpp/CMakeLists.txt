cmake_minimum_required(VERSION 3.22.1)
project(ChiselEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CHISEL_ENABLE_OPENXR "Build OpenXR support" ON)
option(CHISEL_ENABLE_JOLT "Build Jolt Physics" ON)
option(CHISEL_ENABLE_LUA "Build Lua scripting" ON)

include(FetchContent)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
    SYSTEM
    EXCLUDE_FROM_ALL
)

FetchContent_Declare(miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
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

if(CHISEL_ENABLE_LUA)
    FetchContent_Declare(lua
        URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SYSTEM
        EXCLUDE_FROM_ALL
    )
    FetchContent_Declare(luabridge
        GIT_REPOSITORY https://github.com/vinniefalco/LuaBridge.git
        GIT_TAG        2.9
        GIT_SHALLOW    TRUE
        SYSTEM
        EXCLUDE_FROM_ALL
    )
endif()

FetchContent_MakeAvailable(glm stb miniaudio tinygltf)

if(CHISEL_ENABLE_LUA)
    FetchContent_MakeAvailable(lua luabridge)

    file(GLOB LUA_SOURCES CONFIGURE_DEPENDS ${lua_SOURCE_DIR}/src/*.c)
    list(REMOVE_ITEM LUA_SOURCES
        ${lua_SOURCE_DIR}/src/lua.c
        ${lua_SOURCE_DIR}/src/luac.c
    )
    add_library(lua STATIC ${LUA_SOURCES})
    target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)
endif()

if(CHISEL_ENABLE_JOLT)
    set(USE_ASSERTS ON CACHE BOOL "" FORCE)
    set(JPH_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(JPH_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(jolt
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG        v5.2.0
        GIT_SHALLOW    TRUE
        SYSTEM
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(jolt)
    add_subdirectory(${jolt_SOURCE_DIR}/Build ${jolt_BINARY_DIR})
endif()


set(META_OPENXR_SDK_ROOT "C:/Oculus/OpenXR" CACHE PATH "Meta OpenXR SDK root")


set(ENGINE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../..)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    ${ENGINE_ROOT}/src/*.cpp
)

# Exclude desktop-only files that use GLFW
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/platform/Window\\.cpp$")
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/platform/SceneEditor\\.cpp$")

# Add Android-specific sources
list(APPEND ENGINE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/android_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/android_window.cpp
)

add_library(chiselengine SHARED ${ENGINE_SOURCES})

target_include_directories(chiselengine PRIVATE
    ${ENGINE_ROOT}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${stb_SOURCE_DIR}
    ${miniaudio_SOURCE_DIR}
    ${META_OPENXR_SDK_ROOT}/Include
)

if(CHISEL_ENABLE_LUA)
    target_include_directories(chiselengine PRIVATE ${luabridge_SOURCE_DIR}/Source)
endif()

target_compile_definitions(chiselengine PRIVATE
    CHISEL_TARGET_ANDROID
    GLM_ENABLE_EXPERIMENTAL
    XR_USE_PLATFORM_ANDROID
    XR_USE_GRAPHICS_API_OPENGL_ES
)

if(CHISEL_ENABLE_OPENXR)
    target_compile_definitions(chiselengine PRIVATE CHISEL_ENABLE_OPENXR)
endif()

if(CHISEL_ENABLE_JOLT)
    target_compile_definitions(chiselengine PRIVATE CHISEL_ENABLE_JOLT)
    target_include_directories(chiselengine PRIVATE ${jolt_SOURCE_DIR})
endif()

if(CHISEL_ENABLE_LUA)
    target_compile_definitions(chiselengine PRIVATE CHISEL_ENABLE_LUA)
endif()

target_link_libraries(chiselengine
    glm::glm
    tinygltf
    log
    android
    EGL
    GLESv3
    OpenSLES
)

# if(CHISEL_ENABLE_OPENXR)
#     target_link_libraries(chiselengine
#         ${META_OPENXR_SDK_ROOT}/Lib/Android/arm64-v8a/Release/libopenxr_loader.so
#     )
# endif()

if(CHISEL_ENABLE_JOLT)
    target_link_libraries(chiselengine Jolt)
endif()

if(CHISEL_ENABLE_LUA)
    target_link_libraries(chiselengine lua)
endif()