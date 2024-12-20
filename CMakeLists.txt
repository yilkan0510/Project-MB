cmake_minimum_required(VERSION 3.15)
project(CFG_Visualization)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Gather all cpp files from src
file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/logic/*.cpp)

add_executable(Release ${SOURCES}
        src/main.cpp)

add_executable(GUI ${SOURCES}
        src/main_gui.cpp)

target_include_directories(Release PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/imgui
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends
        ${CMAKE_SOURCE_DIR}/libs/glfw/include
        ${CMAKE_SOURCE_DIR}/libs/glew/include
)


target_include_directories(GUI PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/imgui
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends
        ${CMAKE_SOURCE_DIR}/libs/glfw/include
        ${CMAKE_SOURCE_DIR}/libs/glew/include
)

add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW)

target_link_directories(Release PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/glfw/build/src
        ${CMAKE_SOURCE_DIR}/libs/glew/build/lib
)


target_link_directories(GUI PRIVATE
        ${CMAKE_SOURCE_DIR}/libs/glfw/build/src
        ${CMAKE_SOURCE_DIR}/libs/glew/build/lib
)

target_link_libraries(Release
        glfw3        # or glfw depending on your build
        GLEW
        GL
        pthread
        X11
        dl
        m
)


target_link_libraries(GUI
        glfw3        # or glfw depending on your build
        GLEW
        GL
        pthread
        X11
        dl
        m
)

# Include ImGui backend files
file(GLOB IMGUI_BACKEND_SOURCES
        ${CMAKE_SOURCE_DIR}/libs/imgui/*.cpp
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_glfw.cpp
        ${CMAKE_SOURCE_DIR}/libs/imgui/backends/imgui_impl_opengl3.cpp
)

target_sources(Release PRIVATE ${IMGUI_BACKEND_SOURCES})

target_sources(GUI PRIVATE ${IMGUI_BACKEND_SOURCES})
