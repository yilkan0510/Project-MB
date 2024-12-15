#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

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

  // Use Graphviz to render the DOT file into an image
  std::string command = "dot -Tpng " + dotFilename + " -o " + outputImage;
  int result = std::system(command.c_str());
  if (result != 0) {
    std::cerr << "Graphviz rendering failed!" << std::endl;
    return false;
  }
  return true;
}

int main() {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW!" << std::endl;
    return -1;
  }

  // Create a GLFW window
  GLFWwindow* window = glfwCreateWindow(800, 600, "Dear ImGui + Graphviz Test", nullptr, nullptr);
  if (!window) {
    std::cerr << "Failed to create GLFW window!" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // Enable VSync

  // Initialize OpenGL loader (GLAD, GLEW, etc.)
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
    return -1;
  }

  // Initialize Dear ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  // Generate a Graphviz image
  const std::string dotFilename = "test_graph.dot";
  const std::string outputImage = "test_graph.png";
  if (!generateGraphImage(dotFilename, outputImage)) {
    std::cerr << "Failed to generate graph image!" << std::endl;
    return -1;
  }

  // Load the Graphviz image as a texture (placeholder, loading actual texture requires stb_image or similar)
  GLuint graphTexture = 0; // Replace this with actual image loading later

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    // Poll events
    glfwPollEvents();

    // Start Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ImGui window
    ImGui::Begin("Graphviz Test");
    ImGui::Text("This is a simple Dear ImGui + Graphviz test.");
    if (ImGui::Button("Render Graph")) {
      // Render the graph again if the button is pressed
      generateGraphImage(dotFilename, outputImage);
      std::cout << "Graph re-rendered!" << std::endl;
    }
    ImGui::End();

    // Render
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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