cmake_minimum_required(VERSION 3.15)
project(CFG_Visualization)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories for ImGui, GLFW, and Graphviz
include_directories(${CMAKE_SOURCE_DIR}/libs/imgui)
include_directories(${CMAKE_SOURCE_DIR}/libs/glfw/include)

# Add source files
file(GLOB_RECURSE SOURCES
        ${CMAKE_SOURCE_DIR}/src/*.cpp
        ${CMAKE_SOURCE_DIR}/libs/imgui/*.cpp
)

add_executable(CFG_Visualization ${SOURCES})

# Link libraries
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)

# GLFW
pkg_check_modules(GLFW REQUIRED glfw3)
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})
target_link_libraries(CFG_Visualization ${GLFW_LIBRARIES})

# Dear ImGui requires OpenGL and GLFW
target_link_libraries(CFG_Visualization OpenGL::GL)

# Graphviz
find_package(Graphviz REQUIRED)
include_directories(${GRAPHVIZ_INCLUDE_DIR})
target_link_libraries(CFG_Visualization ${GRAPHVIZ_LIBRARIES})

# Compiler warnings (optional, for debugging)
if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(CFG_Visualization PRIVATE -Wall -Wextra -pedantic -Werror)
endif()

# Set working directory in CLion to the project's root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)