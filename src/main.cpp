#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>
#include <string>
#include "GLRParser.h"
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

/*
int main() {
    GLRParser parser("../src/JSON/CFG1.json");
    parser.parse({"a","b","0"});
    return 0;
}
*/

int main() {
  // Test the trivial unambiguous CFG
  CFG cfg_unambiguous("../src/JSON/input-unambiguous.json");
  std::string testString1 = "a";
  std::cout << "Testing ambiguity for unambiguous grammar with string: " << testString1 << std::endl;
  if (cfg_unambiguous.isAmbiguous(testString1)) {
    std::cout << "The CFG is ambiguous for the string \"" << testString1 << "\"" << std::endl;
  } else {
    std::cout << "The CFG is not ambiguous for the string \"" << testString1 << "\"" << std::endl;
  }

  // Test the first ambiguous CFG (no epsilon)
  CFG cfg_ambiguous("../src/JSON/input-ambiguous.json");
  std::string testString2 = "aa";
  std::cout << "\nTesting ambiguity for the first ambiguous grammar with string: " << testString2 << std::endl;
  if (cfg_ambiguous.isAmbiguous(testString2)) {
    std::cout << "The CFG is ambiguous for the string \"" << testString2 << "\"" << std::endl;
  } else {
    std::cout << "The CFG is not ambiguous for the string \"" << testString2 << "\"" << std::endl;
  }

  // Test the second ambiguous CFG (with epsilon)
  CFG cfg_ambiguous2("../src/JSON/input-ambiguous-2.json");
  std::string testString3 = "aa";
  std::cout << "\nTesting ambiguity for the second ambiguous grammar with string: " << testString3 << std::endl;
  if (cfg_ambiguous2.isAmbiguous(testString3)) {
    std::cout << "The CFG is ambiguous for the string \"" << testString3 << "\"" << std::endl;
  } else {
    std::cout << "The CFG is not ambiguous for the string \"" << testString3 << "\"" << std::endl;
  }

  return 0;
}
