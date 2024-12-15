#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Specify OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use Core Profile

    // Create a GLFW window with OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "Basic OpenGL Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-Sync

    // Initialize GLEW
    glewExperimental = GL_TRUE;  // Enable experimental features for GLEW
    GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW! Error: " << glewGetErrorString(glewInitResult) << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Print OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Poll for and process events
        glfwPollEvents();

        // Clear the screen with a color
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // Set a background color
        glClear(GL_COLOR_BUFFER_BIT);          // Clear the color buffer

        // Swap front and back buffers
        glfwSwapBuffers(window);
    }

    // Clean up
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
