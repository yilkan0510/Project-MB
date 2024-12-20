#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <filesystem>
#include <fstream>
#include <cstdio>

// Include your CFG and parser headers
#include "logic/CFG.h"
#include "logic/EarleyParser.h"
#include "logic/GLRParser.h"

// ImGui and backend
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../stb/stb_image.h"

// Forward declarations for GUI helpers
static void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static bool loadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
  int width, height, channels;
  unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
  if (data == NULL)
    return false;

  glGenTextures(1, out_texture);
  glBindTexture(GL_TEXTURE_2D, *out_texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  stbi_image_free(data);

  *out_width = width;
  *out_height = height;
  return true;
}

// Dummy functions for generating dot files. You must implement these:
static void generateDotFileForGrammar(const CFG &cfg, const std::string &filename) {
  // Example: Just show non-terminals and productions as a simple graph
  std::ofstream out(filename);
  out << "digraph G {\n";
  out << "rankdir=LR;\n";
  // Show non-terminals as nodes
  for (auto &nt : cfg.getNonTerminals()) {
    out << "\"" << nt << "\" [shape=box];\n";
  }
  // Show edges from NT to each production body symbol
  for (auto &rule : cfg.getProductionRules()) {
    for (auto &body : rule.second) {
      out << "\"" << rule.first << "\" -> \"" << body << "\";\n";
    }
  }
  out << "}\n";
}

static void generateDotFileForParserState(const EarleyParser &parser, const std::string &filename) {
  // Represent the Earley chart as nodes:
  const auto &chart = parser.getChart();
  std::ofstream out(filename);
  out << "digraph EarleyChart {\n";
  for (size_t i=0; i<chart.size(); i++) {
    out << "node" << i << " [shape=box,label=\"Chart[" << i << "]\\n";
    for (auto &it : chart[i]) {
      std::string dotBody = it.body;
      dotBody.insert(it.dotPos, "•");
      out << it.head << "->" << dotBody << " (start=" << it.startIdx << ")\\l";
    }
    out << "\"];\n";
    if (i>0) {
      out << "node" << (i-1) << "->node" << i << ";\n";
    }
  }
  out << "}\n";
}

static void runDotToPng(const std::string &dotFile, const std::string &pngFile) {
  std::string cmd = "dot -Tpng -o " + pngFile + " " + dotFile;
  system(cmd.c_str());
}

// Globals for GUI state
static char grammarPath[256] = "src/JSON/CFG4.json";
static char inputString[256] = "";
static std::string parseResultEarley = "";
static std::string parseResultGLR = "";

static bool stepByStepEarley = false;
static bool earleyFinished = false;

static bool stepByStepGLR = false;
static bool glrFinished = false;

// Visualization
static bool showGraphWindow = false;
static GLuint graphTexture = 0;
static int graphTexWidth = 0;
static int graphTexHeight = 0;
static std::string currentDotFile = "state.dot";
static std::string currentPngFile = "state.png";

// CFG creation mode
static bool cfgEditorOpen = false;
static std::set<std::string> editorNonTerminals;
static std::set<char> editorTerminals;
static std::map<std::string,std::vector<std::string>> editorProductions;
static char newNonTerminal[16] = "";
static char newTerminal[16] = "";
static char prodHead[16] = "";
static char prodBody[256] = "";

// Import menu
static bool importMenuOpen = false;
static std::string grammarsDir = "../grammars/";
static std::vector<std::string> availableGrammars;

// Export menu
static bool exportMenuOpen = false;
static char exportBaseName[256] = "exported";
static std::string exportDir = "../exports/";

// Current CFG and parsers
static std::unique_ptr<CFG> currentCFG;
static std::unique_ptr<EarleyParser> earleyParser;
static std::unique_ptr<GLRParser> glrParser;

// Helper: Refresh directory listing
static void refreshAvailableGrammars() {
  availableGrammars.clear();
  for (auto &entry : std::filesystem::directory_iterator(grammarsDir)) {
    if (entry.path().extension() == ".json") {
      availableGrammars.push_back(entry.path().filename().string());
    }
  }
}

// Update Graph: generates dot and png from current CFG or parser state
static void updateGraphVisualization() {
  if (stepByStepEarley && earleyParser) {
    // Generate parser state graph
    generateDotFileForParserState(*earleyParser, currentDotFile);
  } else if (currentCFG) {
    // Show grammar graph if not in step by step mode
    generateDotFileForGrammar(*currentCFG, currentDotFile);
  } else {
    // If no CFG, generate empty graph?
    std::ofstream out(currentDotFile);
    out << "digraph G {\nempty[label=\"No CFG\"];\n}\n";
  }

  runDotToPng(currentDotFile, currentPngFile);

  if (graphTexture) {
    glDeleteTextures(1, &graphTexture);
    graphTexture = 0;
  }

  loadTextureFromFile(currentPngFile.c_str(), &graphTexture, &graphTexWidth, &graphTexHeight);
}

int main(int, char**)
{
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,0);

  GLFWwindow* window = glfwCreateWindow(1280,720,"CFG Visualization",NULL,NULL);
  if (window==NULL) {
    fprintf(stderr,"Failed to create window\n");
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  if (glewInit()!=GLEW_OK) {
    fprintf(stderr,"Failed to init GLEW\n");
    return 1;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window,true);
  ImGui_ImplOpenGL3_Init("#version 130");

  refreshAvailableGrammars();
  updateGraphVisualization();

  while (!glfwWindowShouldClose(window))
  {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main Menu Bar
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Import")) {
          importMenuOpen = true;
        }
        if (ImGui::MenuItem("Export")) {
          exportMenuOpen = true;
        }
        if (ImGui::MenuItem("Edit CFG")) {
          cfgEditorOpen = true;
        }
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
    }

    // Import Window
    if (importMenuOpen) {
      ImGui::Begin("Import CFG", &importMenuOpen);
      ImGui::Text("Select a grammar from %s", grammarsDir.c_str());
      for (auto &g : availableGrammars) {
        if (ImGui::Button(g.c_str())) {
          std::string path = grammarsDir + g;
          strncpy(grammarPath, path.c_str(), sizeof(grammarPath));
          importMenuOpen = false;
        }
      }
      ImGui::End();
    }

    // Export Window
    if (exportMenuOpen) {
      ImGui::Begin("Export", &exportMenuOpen);
      ImGui::InputText("Base Name", exportBaseName, IM_ARRAYSIZE(exportBaseName));
      if (ImGui::Button("Export Now")) {
        // Export current CFG JSON
        if (currentCFG) {
          std::string jsonExport = exportDir + std::string(exportBaseName) + ".json";
          // Saving currentCFG as JSON
          // This requires code to serialize CFG to JSON, omitted for brevity
          // Implement a function to save CFG to JSON:
          // saveCFGToJSON(*currentCFG, jsonExport);

          // Export dot
          std::string dotExport = exportDir + std::string(exportBaseName) + ".dot";
          if (stepByStepEarley && earleyParser) {
            generateDotFileForParserState(*earleyParser, dotExport);
          } else if (currentCFG) {
            generateDotFileForGrammar(*currentCFG, dotExport);
          }

          // Export png via dot
          std::string pngExport = exportDir + std::string(exportBaseName) + ".png";
          std::string cmd = "dot -Tpng -o " + pngExport + " " + dotExport;
          system(cmd.c_str());
        }
        exportMenuOpen = false;
      }
      ImGui::End();
    }

    // CFG Editor
    if (cfgEditorOpen) {
      ImGui::Begin("CFG Editor", &cfgEditorOpen);
      ImGui::Text("Non-terminals:");
      for (auto &nt : editorNonTerminals) {
        ImGui::BulletText("%s", nt.c_str());
      }
      ImGui::InputText("Add Non-terminal", newNonTerminal, IM_ARRAYSIZE(newNonTerminal));
      if (ImGui::Button("Add NT")) {
        if (strlen(newNonTerminal)>0) {
          editorNonTerminals.insert(newNonTerminal);
          newNonTerminal[0]='\0';
        }
      }

      ImGui::Separator();
      ImGui::Text("Terminals:");
      for (auto t : editorTerminals) {
        ImGui::BulletText("%c", t);
      }
      ImGui::InputText("Add Terminal", newTerminal, IM_ARRAYSIZE(newTerminal));
      if (ImGui::Button("Add T")) {
        if (strlen(newTerminal)>0 && strlen(newTerminal)==1) {
          editorTerminals.insert(newTerminal[0]);
          newTerminal[0]='\0';
        }
      }

      ImGui::Separator();
      ImGui::Text("Productions:");
      for (auto &rule : editorProductions) {
        for (auto &body : rule.second) {
          ImGui::BulletText("%s -> %s", rule.first.c_str(), body.c_str());
        }
      }

      ImGui::InputText("Prod Head", prodHead, IM_ARRAYSIZE(prodHead));
      ImGui::InputText("Prod Body (concatenated symbols)", prodBody, IM_ARRAYSIZE(prodBody));
      if (ImGui::Button("Add Production")) {
        if (strlen(prodHead)>0 && editorNonTerminals.count(prodHead)>0) {
          editorProductions[prodHead].push_back(std::string(prodBody));
          prodHead[0]='\0';
          prodBody[0]='\0';
        }
      }

      if (ImGui::Button("Save CFG")) {
        // Construct a CFG object from editor data
        // This requires code similar to loading from JSON, but in reverse
        // Just create a CFG file manually:
        std::string savePath = grammarsDir + std::string("edited.json");
        // Implement saveEditorCFGToJSON(editorNonTerminals,editorTerminals,editorProductions,savePath);

        // Then load it into currentCFG
        currentCFG = std::make_unique<CFG>(savePath);
        earleyParser = std::make_unique<EarleyParser>(*currentCFG);
        glrParser = std::make_unique<GLRParser>(*currentCFG);
        updateGraphVisualization();
      }

      ImGui::End();
    }

    // Main window for parsing
    ImGui::Begin("Main Controls");
    ImGui::InputText("CFG File Path", grammarPath, IM_ARRAYSIZE(grammarPath));
    if (ImGui::Button("Load Grammar")) {
      try {
        currentCFG = std::make_unique<CFG>(grammarPath);
        earleyParser = std::make_unique<EarleyParser>(*currentCFG);
        glrParser = std::make_unique<GLRParser>(*currentCFG);
        parseResultEarley = "Grammar Loaded!";
        parseResultGLR = "Grammar Loaded!";
        stepByStepEarley = false;
        earleyFinished = false;
        stepByStepGLR = false;
        glrFinished = false;
        updateGraphVisualization();
      } catch(...) {
        parseResultEarley = "Failed to load grammar";
        parseResultGLR = "Failed to load grammar";
      }
    }

    ImGui::InputText("Input String", inputString, IM_ARRAYSIZE(inputString));

    // Parser Buttons
    if (ImGui::Button("Earley Parse (Full)")) {
      if (earleyParser) {
        bool res = earleyParser->parse(inputString);
        parseResultEarley = res ? "Accepted" : "Rejected";
        updateGraphVisualization();
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Earley Step-by-Step")) {
      if (earleyParser) {
        earleyParser->reset(inputString);
        stepByStepEarley = true;
        earleyFinished = false;
        updateGraphVisualization();
      }
    }

    if (stepByStepEarley && !earleyFinished) {
      if (ImGui::Button("Next Step (Earley)")) {
        bool cont = earleyParser->nextStep();
        if (!cont) {
          earleyFinished = true;
          parseResultEarley = earleyParser->isAccepted() ? "Accepted" : "Rejected";
        }
        updateGraphVisualization();
      }
    }

    ImGui::Separator();

    if (ImGui::Button("GLR Parse (Full)")) {
      // Implement GLR full parse:
      // bool res = glrParser->parse(vector_of_tokens);
      // parseResultGLR = res ? "Accepted" : "Rejected";
      // Just a placeholder since tokenization not shown:
      parseResultGLR = "Not Implemented Yet";
    }
    ImGui::SameLine();
    if (ImGui::Button("GLR Step-by-Step")) {
      stepByStepGLR = true; // Similarly handle GLR step simulation
                            // Not fully implemented
    }

    if (stepByStepGLR && !glrFinished) {
      // Add Next Step for GLR once implemented
      ImGui::Text("GLR Step-by-step not fully implemented.");
    }

    ImGui::Separator();
    ImGui::Text("Earley result: %s", parseResultEarley.c_str());
    ImGui::Text("GLR result:   %s", parseResultGLR.c_str());

    if (ImGui::Button("Show Graph")) {
      showGraphWindow = true;
    }

    ImGui::End();

    // Graph Window
    if (showGraphWindow) {
      ImGui::Begin("Graph Visualization", &showGraphWindow);
      if (graphTexture) {
        ImGui::Text("Graph (width=%d, height=%d):", graphTexWidth, graphTexHeight);
        ImGui::Image((ImTextureID)(intptr_t)graphTexture, ImVec2((float)graphTexWidth, (float)graphTexHeight));

      } else {
        ImGui::Text("No graph available.");
      }
      ImGui::End();
    }

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window,&display_w,&display_h);
    glViewport(0,0,display_w,display_h);
    glClearColor(0.45f,0.55f,0.60f,1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
