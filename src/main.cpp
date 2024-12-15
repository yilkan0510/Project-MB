#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>
#include <string>

// stb_image
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"

// Function to generate a Graphviz DOT file and render it to a PNG
bool generateGraphImage(const std::string& dotFilename, const std::string& outputImage) {
    // Write a simple DOT file
    std::ofstream dotFile(dotFilename);
    if (!dotFile) {
        std::cerr << "Failed to create DOT file!" << std::endl;
        return false;
    }
    dotFile << "digraph G {\n"
            << "  A -> B;\n"
            << "  B -> C;\n"
            << "  C -> A;\n"
            << "}\n";
    dotFile.close();

    // Use Graphviz to render the DOT file into a PNG
    std::string command = "dot -Tpng " + dotFilename + " -o " + outputImage;
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Graphviz rendering failed!" << std::endl;
        return false;
    }
    return true;
}

// OpenGL utility: Load a texture from a PNG file
GLuint loadTexture(const std::string& imagePath) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Load image data
    int width, height, channels;
    unsigned char* data = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);
    if (!data) {
        std::cerr << "Failed to load image: " << imagePath << std::endl;
        return 0;
    }

    // Generate texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    return textureID;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Specify OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Complex Test: All Libraries", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable V-Sync

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    glGetError();  // Clear any existing OpenGL errors
    GLenum glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW! Error: " << glewGetErrorString(glewInitResult) << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Print OpenGL version
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    // Generate a Graphviz image
    const std::string dotFilename = "graph.dot";
    const std::string outputImage = "graph.png";
    if (!generateGraphImage(dotFilename, outputImage)) {
        std::cerr << "Graphviz test failed!" << std::endl;
    }

    // Load the Graphviz-generated image as an OpenGL texture
    GLuint graphTexture = loadTexture(outputImage);

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start a new frame for Dear ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render a Dear ImGui window
        ImGui::Begin("Library Test");
        ImGui::Text("OpenGL Version: %s", glGetString(GL_VERSION));
        ImGui::Text("Graphviz Test: Generated graph.dot -> graph.png");
        if (graphTexture) {
            ImGui::Text("Graphviz Image:");
            ImGui::Image((ImTextureID)(intptr_t)graphTexture, ImVec2(200, 200));
        } else {
            ImGui::Text("Failed to load Graphviz image.");
        }
        ImGui::End();

        // Render Dear ImGui data
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
