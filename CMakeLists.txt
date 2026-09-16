cmake_minimum_required(VERSION 3.15)
project(ChiselEngine VERSION 1.0 LANGUAGES CXX)

# Use C++17 or C++20 for modern features and Jolt support
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(
    include
    Libraries/include
)

# Define source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "Core/*.cpp"
)

# Add executable
add_executable(ChiselEngine ${SOURCES})

# Link Libraries
target_link_libraries(ChiselEngine 
    PRIVATE 
    opengl32
    user32
    gdi32
    # Replace these with the actual paths to your .lib files
    # ${CMAKE_SOURCE_DIR}/Libraries/lib/jolt.lib
    # ${CMAKE_SOURCE_DIR}/Libraries/lib/assimp.lib
)

# Copy DLLs to bin folder after build (Windows)
add_custom_command(TARGET ChiselEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/Libraries/bin"
    "$<TARGET_FILE_DIR:ChiselEngine>"
)
