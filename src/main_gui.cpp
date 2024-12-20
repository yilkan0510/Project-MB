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

// stb_image for image loading (implementation in stb_image_impl.cpp)
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
      // Represent body as a single string for simplicity
      out << "\"" << rule.first << "\" -> \"" << body << "\";\n";
    }
  }
  out << "}\n";
}

static void generateDotFileForParserState(const EarleyParser &parser, const std::string &filename) {
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
static char grammarPath[256] = "../src/JSON/CFG4.json";
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

// CFG Maker (previously CFG Editor)
static bool cfgMakerOpen = false;
static std::set<std::string> editorNonTerminals;
static std::set<char> editorTerminals;
static std::map<std::string,std::vector<std::string>> editorProductions;
static char newNonTerminal[16] = "";
static char newTerminal[16] = "";
static char prodHead[16] = "";
static char prodBody[256] = "";
static char editorStartSymbol[16] = "";

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
    generateDotFileForParserState(*earleyParser, currentDotFile);
  } else if (currentCFG) {
    generateDotFileForGrammar(*currentCFG, currentDotFile);
  } else {
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

// Save the CFG made in the CFG Maker to JSON
static void saveEditorCFGToJSON(const std::string &path) {
  // Build JSON-like structure
  // Ensure that editorStartSymbol is a valid non-terminal
  // For simplicity, no error checking here
  std::ofstream out(path);
  out << "{\n";
  // Variables
  out << "  \"Variables\": [";
  {
    bool first=true;
    for (auto &nt : editorNonTerminals) {
      if (!first) out << ", ";
      out << "\"" << nt << "\"";
      first=false;
    }
  }
  out << "],\n";

  // Terminals
  out << "  \"Terminals\": [";
  {
    bool first=true;
    for (auto t : editorTerminals) {
      if (!first) out << ", ";
      out << "\"" << t << "\"";
      first=false;
    }
  }
  out << "],\n";

  // Productions
  out << "  \"Productions\": [\n";
  {
    bool firstOuter=true;
    for (auto &rule : editorProductions) {
      for (auto &body : rule.second) {
        if (!firstOuter) out << ",\n";
        out << "    {\"head\": \"" << rule.first << "\", \"body\": [";
        // body is a single string of concatenated symbols
        // We must split it into separate symbols if needed
        // For simplicity, assume each symbol is one character
        bool firstInner=true;
        for (size_t i=0; i<body.size(); i++) {
          if (!firstInner) out << ", ";
          std::string sym(1, body[i]);
          out << "\"" << sym << "\"";
          firstInner=false;
        }
        out << "]}";
        firstOuter=false;
      }
    }
  }
  out << "\n  ],\n";

  // Start
  out << "  \"Start\": \"" << editorStartSymbol << "\"\n";
  out << "}\n";
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

    // Create a dockspace
    {
      static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
      ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                      ImGuiWindowFlags_NoNavFocus;

      ImGuiViewport* viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::SetNextWindowViewport(viewport->ID);

      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::Begin("DockSpace Window", nullptr, window_flags);
      ImGui::PopStyleVar(2);

      ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f,0.0f), dockspace_flags);

      ImGui::End();
    }


    // Main Menu Bar
    if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Import")) {
          importMenuOpen = true;
        }
        if (ImGui::MenuItem("Export")) {
          exportMenuOpen = true;
        }
        if (ImGui::MenuItem("CFG Maker")) {
          cfgMakerOpen = true;
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
        if (currentCFG) {
          // Export JSON
          std::string jsonExport = exportDir + std::string(exportBaseName) + ".json";
          // Implement a function to save the currentCFG to JSON
          // Since we have a CFG object, mimic the structure of your saveEditorCFGToJSON:
          // (This requires you to write a similar function that takes currentCFG and writes it out)

          // Example (pseudo-code):
          std::ofstream out(jsonExport);
          out << "{\n";
          // Write Variables
          out << "  \"Variables\": [";
          bool first=true;
          for (auto &v : currentCFG->getNonTerminals()) {
            if (!first) out << ", ";
            out << "\"" << v << "\"";
            first=false;
          }
          out << "],\n";

          // Write Terminals
          out << "  \"Terminals\": [";
          first=true;
          for (auto t : currentCFG->getTerminals()) {
            if (!first) out << ", ";
            out << "\"" << t << "\"";
            first=false;
          }
          out << "],\n";

          // Write Productions
          out << "  \"Productions\": [\n";
          {
            bool firstOuter = true;
            for (auto &rule : currentCFG->getProductionRules()) {
              for (auto &body : rule.second) {
                if (!firstOuter) out << ",\n";
                out << "    {\"head\": \"" << rule.first << "\", \"body\": [";
                bool firstInner=true;
                for (auto &c : body) {
                  if (!firstInner) out << ", ";
                  out << "\"" << c << "\"";
                  firstInner=false;
                }
                out << "]}";
                firstOuter = false;
              }
            }
          }
          out << "\n  ],\n";

          // Write Start
          out << "  \"Start\": \"" << currentCFG->getStartSymbol() << "\"\n";
          out << "}\n";

          // Export DOT and PNG
          std::string dotExport = exportDir + std::string(exportBaseName) + ".dot";
          if (stepByStepEarley && earleyParser) {
            generateDotFileForParserState(*earleyParser, dotExport);
          } else if (currentCFG) {
            generateDotFileForGrammar(*currentCFG, dotExport);
          }

          std::string pngExport = exportDir + std::string(exportBaseName) + ".png";
          std::string cmd = "dot -Tpng -o " + pngExport + " " + dotExport;
          system(cmd.c_str());
        }
        exportMenuOpen = false;
      }
      ImGui::End();
    }

    // CFG Maker Window
    if (cfgMakerOpen) {
      ImGui::Begin("CFG Maker", &cfgMakerOpen);
      ImGui::Text("Create your CFG here.");
      // Start Symbol Section
      ImGui::Separator();
      ImGui::Text("Start Symbol:");

      if (strlen(editorStartSymbol) > 0) {
        // A start symbol is already set, display it with a bullet and X button
        ImGui::BulletText("%s (Start)", editorStartSymbol);
        ImGui::SameLine();
        if (ImGui::Button("X##StartSymbol")) {
          // User clicked X to remove the start symbol
          // If this start symbol is in non-terminals, remove it from there as well
          std::string sSymbol(editorStartSymbol);
          if (editorNonTerminals.count(sSymbol)) {
            editorNonTerminals.erase(sSymbol);
          }
          editorStartSymbol[0] = '\0'; // Clear the start symbol
        }
        ImGui::Text("To change the start symbol, remove it first, then add a new one.");
      } else {
        // No start symbol set yet, allow user to input one and add it
        static char tempStartSymbol[16] = "";
        ImGui::InputText("Enter Start Symbol", tempStartSymbol, IM_ARRAYSIZE(tempStartSymbol));
        if (ImGui::Button("Add Start Symbol")) {
          if (strlen(tempStartSymbol) > 0) {
            std::string sSymbol(tempStartSymbol);
            // Ensure start symbol not in terminals
            // If sSymbol is one character and is in terminals, remove it
            if (sSymbol.size() == 1 && editorTerminals.count(sSymbol[0])) {
              // Remove from terminals since start symbol must be a non-terminal
              editorTerminals.erase(sSymbol[0]);
            }

            // Add the start symbol to non-terminals if not present
            if (!editorNonTerminals.count(sSymbol)) {
              editorNonTerminals.insert(sSymbol);
            }

            // Set the editorStartSymbol
            strncpy(editorStartSymbol, tempStartSymbol, IM_ARRAYSIZE(editorStartSymbol));
            tempStartSymbol[0] = '\0';
          }
        }
      }

      // Now continue with your Non-terminals, Terminals, and Productions code as before...


      ImGui::Separator();
      ImGui::Text("Non-terminals:");
      {
        // Non-terminals display and deletion
        std::vector<std::string> ntemp(editorNonTerminals.begin(), editorNonTerminals.end());
        for (auto &nt : ntemp) {
          ImGui::BulletText("%s", nt.c_str());
          ImGui::SameLine();

          std::string delButton = "X##NT_" + nt;
          if (ImGui::Button(delButton.c_str())) {
            // If this non-terminal is the start symbol, clear it
            if (nt == editorStartSymbol) {
              editorStartSymbol[0] = '\0'; // Clear the start symbol since we removed it
            }
            editorNonTerminals.erase(nt);
            break;
          }
        }
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
      {
        std::vector<char> ttemp(editorTerminals.begin(), editorTerminals.end());
        for (auto t : ttemp) {
          ImGui::BulletText("%c", t);
          ImGui::SameLine();
          std::string delButton = "X##T_";
          delButton.push_back(t);
          if (ImGui::Button(delButton.c_str())) {
            editorTerminals.erase(t);
            break;
          }
        }
      }
      ImGui::InputText("Add Terminal", newTerminal, IM_ARRAYSIZE(newTerminal));
      if (ImGui::Button("Add T")) {
        for (const auto &nt : editorNonTerminals) {
          if (std::string(newTerminal) == nt) {
            newTerminal[0]='\0';
            break;
          }
        }
        if (strlen(newTerminal)==1) {
          char t = newTerminal[0];
          std::string termSymbol(1, t);

          // Check if the termSymbol is the same as the current start symbol
          if (termSymbol == editorStartSymbol) {
            // show a warning or just skip adding
            // For example, we can print a message in ImGui:
            ImGui::TextColored(ImVec4(1,0,0,1), "Cannot add start symbol as terminal!");
            // do NOT insert into editorTerminals
          } else {
            editorTerminals.insert(t);
            newTerminal[0]='\0';
          }
        }
      }

      ImGui::Separator();
      ImGui::Text("Productions:");
      {
        // We'll show "head -> body"
        // Also allow deletion of each production
        // Careful removal: We'll create a new structure after removals

        // We'll store a list of (head,body) to handle removal
        struct ProdItem {
          std::string head;
          std::string body;
        };
        std::vector<ProdItem> allProds;
        for (auto &rule : editorProductions) {
          for (auto &bd : rule.second) {
            allProds.push_back({rule.first, bd});
          }
        }

        bool removedOne = false;
        ProdItem toRemove;
        for (auto &p : allProds) {
          ImGui::BulletText("%s -> %s", p.head.c_str(), p.body.c_str());
          ImGui::SameLine();
          std::string delButton = "X##P_" + p.head + "_" + p.body;
          if (ImGui::Button(delButton.c_str())) {
            removedOne = true;
            toRemove = p;
            break;
          }
        }

        if (removedOne) {
          // Remove toRemove from editorProductions
          auto &vec = editorProductions[toRemove.head];
          vec.erase(std::remove(vec.begin(), vec.end(), toRemove.body), vec.end());
          if (vec.empty()) {
            editorProductions.erase(toRemove.head);
          }
        }
      }

      ImGui::InputText("Prod Head", prodHead, IM_ARRAYSIZE(prodHead));
      ImGui::InputText("Prod Body (symbols concatenated)", prodBody, IM_ARRAYSIZE(prodBody));
      if (ImGui::Button("Add Production")) {
        if (strlen(prodHead)>0 && editorNonTerminals.count(prodHead)>0) {
          editorProductions[prodHead].push_back(std::string(prodBody));
          prodHead[0]='\0';
          prodBody[0]='\0';
        }
      }

      ImGui::Separator();
      if (ImGui::Button("Save as JSON |CreatedJson.json| ")) {
        std::string savePath = grammarsDir + std::string("CreatedJson.json");
        saveEditorCFGToJSON(savePath);

        // Load it into currentCFG
        try {
          currentCFG = std::make_unique<CFG>(savePath);
          earleyParser = std::make_unique<EarleyParser>(*currentCFG);
          glrParser = std::make_unique<GLRParser>(*currentCFG);
          updateGraphVisualization();

          // After saving and loading, refresh the import menu
          refreshAvailableGrammars();
        } catch(...) {
          // handle error if any
        }
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
      if (glrParser) {
        glrParser->reset(inputString);
        // Run until done
        while (!glrParser->isDone()) {
          glrParser->nextStep();
        }
        parseResultGLR = glrParser->isAccepted() ? "Accepted" : "Rejected";
        updateGraphVisualization();
      }
    }
    ImGui::SameLine();

    if (ImGui::Button("GLR Step-by-Step")) {
      if (glrParser) {
        glrParser->reset(inputString);
        stepByStepGLR = true;
        glrFinished = false;
        updateGraphVisualization();
      }
    }

    if (stepByStepGLR && !glrFinished) {
      if (ImGui::Button("Next Step (GLR)")) {
        bool cont = glrParser->nextStep();
        if (!cont) {
          glrFinished = true;
          parseResultGLR = glrParser->isAccepted() ? "Accepted" : "Rejected";
        }
        updateGraphVisualization();
      }
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