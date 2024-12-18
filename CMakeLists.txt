cmake_minimum_required(VERSION 3.15)
project(CFG_Visualization)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories for all libraries
include_directories(
        ${CMAKE_SOURCE_DIR}/libs/imgui
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends
        ${CMAKE_SOURCE_DIR}/libs/glfw/include
        ${CMAKE_SOURCE_DIR}/libs/glew/include  # GLEW headers
)

# Define GLEW as the OpenGL loader for Dear ImGui
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)


# Add source files
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

# Create the executable
add_executable(CFG_Visualization ${SOURCES}
        src/logic/EarleyParser.cpp
        src/logic/EarleyParser.h)

# Link directories for libraries
target_link_directories(CFG_Visualization PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/glfw/build/src      # Path to GLFW static library
        ${CMAKE_SOURCE_DIR}/libs/glew/build/lib      # Path to GLEW static library
)

# Link libraries
target_link_libraries(CFG_Visualization
        glfw3                  # GLFW static library
        GLEW                   # GLEW static library
        GL                     # OpenGL
        pthread                # Required for GLFW
        X11                    # X Window System for Linux
        dl                     # Dynamic linker for Linux
        m                      # Math library
)

# Link Dear ImGui backend files
file(GLOB IMGUI_BACKEND_SOURCES
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.cpp
        ${CMAKE_SOURCE_DIR}/libs/imgui/*.cpp
)
target_sources(CFG_Visualization PRIVATE ${IMGUI_BACKEND_SOURCES})