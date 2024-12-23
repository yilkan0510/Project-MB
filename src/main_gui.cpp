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

// ================ Fix #1: Potentially your CFG constructor might fail ============
// Make sure your CFG code can handle multi-character variables "A","B","S"
// and single-character terminals 'a','b','c' without error.
// That code isn't shown here, so we assume it's correct or you will fix it.

//////////////////////////////////////////////////////////////////////////////////////
// IMGUI helpers
//////////////////////////////////////////////////////////////////////////////////////
static void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

static bool loadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height) {
  int width, height, channels;
  unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
  if (data == NULL) return false;

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

//////////////////////////////////////////////////////////////////////////////////////
// Dot generation
//////////////////////////////////////////////////////////////////////////////////////

// For a plain CFG
static void generateDotFileForGrammar(const CFG &cfg, const std::string &filename) {
  std::ofstream out(filename);
  if(!out){
    std::cerr << "Cannot open " << filename << " for writing CFG.\n";
    return;
  }

  out << "digraph G {\n";
  out << "  rankdir=LR;\n";

  // Show non-terminals as box nodes
  for (auto &nt : cfg.getNonTerminals()) {
    out << "  \"" << nt << "\" [shape=box];\n";
  }
  // Show edges from NT to each production body symbol
  for (auto &rule : cfg.getProductionRules()) {
    for (auto &body : rule.second) {
      // Represent body as a single string
      out << "  \"" << rule.first << "\" -> \"" << body << "\";\n";
    }
  }
  out << "}\n";
  out.close();
}

// For Earley
static void generateDotFileForParserState(const EarleyParser &parser, const std::string &filename) {
  const auto &chart = parser.getChart();
  std::ofstream out(filename);
  if (!out) {
    std::cerr << "Cannot open " << filename << " for writing Earley.\n";
    return;
  }

  out << "digraph EarleyChart {\n";
  out << "  rankdir=LR;\n\n";

  std::vector<std::string> clusterDummies;

  for (size_t i = 0; i < chart.size(); i++) {
    out << "  subgraph cluster_" << i << " {\n";
    out << "    label = \"Chart[" << i << "]\";\n";
    out << "    color=black;\n";
    out << "    style=\"rounded\";\n\n";

    std::string dummyName = "clusterDummy_" + std::to_string(i);
    out << "    " << dummyName << " [shape=point, label=\"\", style=invis];\n";
    clusterDummies.push_back(dummyName);

    int itemIndex = 0;
    for (const auto &item : chart[i]) {
      if (item.head == "S'") continue; // skip augmented

      // Node
      std::string nodeName = "Item_" + std::to_string(i) + "_" + std::to_string(itemIndex);

      std::string dotBody = item.body;
      if (item.dotPos <= dotBody.size()) {
        dotBody.insert(item.dotPos, "•");
      }
      std::string itemLabel = item.head + " -> " + dotBody + " (start=" + std::to_string(item.startIdx) + ")";

      std::string fillColor = "white";
      if (item.dotPos == 0) fillColor = "red";
      else if (item.dotPos < item.body.size()) fillColor = "yellow";
      else fillColor = "green";

      out << "    " << nodeName
          << " [shape=circle, style=filled, fillcolor=" << fillColor
          << ", label=\"" << itemLabel << "\"];\n";

      itemIndex++;
    }

    out << "  }\n\n";
  }

  for (size_t i = 1; i < chart.size(); i++) {
    out << "  " << clusterDummies[i-1] << " -> " << clusterDummies[i]
        << " [style=bold, color=black, penwidth=2.0];\n";
  }

  if (parser.isDone()) {
    bool accepted = parser.isAccepted();
    std::string finalNodeName = accepted ? "Accepted" : "Rejected";
    std::string shape = accepted ? "doublecircle" : "octagon";
    std::string color = accepted ? "green" : "grey";
    out << "  " << finalNodeName
        << " [shape=" << shape << ", style=filled, fillcolor=" << color
        << ", label=\"" << finalNodeName << "\"];\n";

    if (!chart.empty()) {
      size_t lastIndex = chart.size() - 1;
      out << "  " << clusterDummies[lastIndex] << " -> "
          << finalNodeName << " [penwidth=2.0];\n";
    }
  }

  out << "}\n";
  out.close();
}

// For GLR
void generateDotFileForGLRParser(const GLRParser &parser, const std::string &filename) {
  std::ofstream out(filename);
  if (!out) {
    std::cerr << "Cannot open " << filename << " for writing GLR.\n";
    return;
  }

  // Minimal placeholder DOT
  out << "digraph GLR {\n";
  out << "  rankdir=LR;\n\n";
  out << "  // ----- PLACEHOLDER GLR GRAPHVIZ -----\n";
  out << "  // Replace this with your actual GLR visualization later.\n\n";

  // A single node or message to show that the functionality is not yet implemented.
  out << "  PlaceholderNode [shape=box, style=filled, fillcolor=\"#CCCCCC\", label=\"GLR Parser Visualization\\n(Under Construction)\"];\n";
  out << "}\n";

  out.close();
}


// Convert dot to png
static void runDotToPng(const std::string &dotFile, const std::string &pngFile) {
  std::string cmd = "dot -Tpng -o " + pngFile + " " + dotFile;
  system(cmd.c_str());
}

//////////////////////////////////////////////////////////////////////////////////////
// Global State
//////////////////////////////////////////////////////////////////////////////////////
static char grammarPath[256] = "../src/JSON/CFG4.json";
static char inputString[256] = "";
static std::string parseResultEarley = "";
static std::string parseResultGLR = "";

static bool stepByStepEarley = false;
static bool earleyFinished = false;

static bool stepByStepGLR = false;
static bool glrFinished = false;

static bool showGraphWindow = false;
static GLuint graphTexture = 0;
static int graphTexWidth = 0;
static int graphTexHeight = 0;
static std::string currentDotFile = "state.dot";
static std::string currentPngFile = "state.png";

static bool cfgMakerOpen = false;
static std::set<std::string> editorNonTerminals;
static std::set<char> editorTerminals;
static std::map<std::string,std::vector<std::string>> editorProductions;
static char newNonTerminal[16] = "";
static char newTerminal[16] = "";
static char prodHead[16] = "";
static char prodBody[256] = "";
static char editorStartSymbol[16] = "";

static bool importMenuOpen = false;
static std::string grammarsDir = "../grammars/";
static std::vector<std::string> availableGrammars;

static bool exportMenuOpen = false;
static char exportBaseName[256] = "exported";
static std::string exportDir = "../exports/";

static std::unique_ptr<CFG> currentCFG;
static std::unique_ptr<EarleyParser> earleyParser;
static std::unique_ptr<GLRParser> glrParser;

static bool showLegendWindow = false;

static int exportChoice = 0; // 0=Grammar, 1=Earley, 2=GLR

//////////////////////////////////////////////////////////////////////////////////////
// Refresh listing
//////////////////////////////////////////////////////////////////////////////////////
static void refreshAvailableGrammars() {
  availableGrammars.clear();
  for (auto &entry : std::filesystem::directory_iterator(grammarsDir)) {
    if (entry.path().extension() == ".json") {
      availableGrammars.push_back(entry.path().filename().string());
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////////
// Update Graph Visualization
//////////////////////////////////////////////////////////////////////////////////////
static void updateGraphVisualization() {
  // 1) If Earley step mode or done => generateEarley
  if (earleyParser && (stepByStepEarley || earleyParser->isDone())) {
    generateDotFileForParserState(*earleyParser, currentDotFile);

    // 2) else if no earley step => do grammar
  } else if (currentCFG) {
    generateDotFileForGrammar(*currentCFG, currentDotFile);
  }

  // 3) If GLR step mode or done => generate GLR
  if (glrParser && (stepByStepGLR || glrParser->isDone())) {
    generateDotFileForGLRParser(*glrParser, currentDotFile);
  }
  // else if not step glr => do nothing special for glr

  // If none => do final else
  else if (earleyParser && (stepByStepEarley || earleyParser->isDone())) {
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

//////////////////////////////////////////////////////////////////////////////////////
// Save CFG
//////////////////////////////////////////////////////////////////////////////////////
static void saveEditorCFGToJSON(const std::string &path) {
  // Build JSON
  std::ofstream out(path);
  if(!out){
    std::cerr << "Cannot open " << path << " to write JSON.\n";
    return;
  }

  out << "{\n";
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

  out << "  \"Productions\": [\n";
  {
    bool firstOuter=true;
    for (auto &rule : editorProductions) {
      for (auto &body : rule.second) {
        if (!firstOuter) out << ",\n";
        out << "    {\"head\": \"" << rule.first << "\", \"body\": [";
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

  out << "  \"Start\": \"" << editorStartSymbol << "\"\n";
  out << "}\n";
}

//////////////////////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////////////////////
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

  // Main loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Dockspace
    {
      static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
      ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking
                                      | ImGuiWindowFlags_NoTitleBar
                                      | ImGuiWindowFlags_NoCollapse
                                      | ImGuiWindowFlags_NoResize
                                      | ImGuiWindowFlags_NoMove
                                      | ImGuiWindowFlags_NoBringToFrontOnFocus
                                      | ImGuiWindowFlags_NoNavFocus;

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

    // Main menu bar
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

    // Export Window
    if (exportMenuOpen) {
      ImGui::Begin("Export", &exportMenuOpen);

      // 1) Base name input
      ImGui::InputText("Base Name", exportBaseName, IM_ARRAYSIZE(exportBaseName));

      // 2) Let user pick what to export
      static int exportChoice = 0; // 0=Grammar, 1=Earley, 2=GLR
      ImGui::Text("Select Graph to Export:");
      ImGui::RadioButton("Grammar", &exportChoice, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Earley", &exportChoice, 1);
      ImGui::SameLine();
      ImGui::RadioButton("GLR", &exportChoice, 2);

      // 3) Export button
      if (ImGui::Button("Export Now")) {
        if (currentCFG) {
          // 3a) Build JSON export of the CFG
          std::string jsonExport = exportDir + std::string(exportBaseName) + ".json";
          {
            std::ofstream out(jsonExport);
            if (!out) {
              std::cerr << "Failed to open file " << jsonExport << " for writing!\n";
            } else {
              // Dump the CFG to JSON
              out << "{\n";
              out << "  \"Variables\": [";
              bool first = true;
              for (const auto &v : currentCFG->getNonTerminals()) {
                if (!first) out << ", ";
                out << "\"" << v << "\"";
                first = false;
              }
              out << "],\n";

              out << "  \"Terminals\": [";
              first = true;
              for (auto t : currentCFG->getTerminals()) {
                if (!first) out << ", ";
                out << "\"" << t << "\"";
                first = false;
              }
              out << "],\n";

              out << "  \"Productions\": [\n";
              bool firstOuter = true;
              for (auto &rule : currentCFG->getProductionRules()) {
                for (auto &body : rule.second) {
                  if (!firstOuter) out << ",\n";
                  out << "    {\"head\": \"" << rule.first << "\", \"body\": [";
                  bool firstInner = true;
                  for (char c : body) {
                    if (!firstInner) out << ", ";
                    out << "\"" << c << "\"";
                    firstInner = false;
                  }
                  out << "]}";
                  firstOuter = false;
                }
              }
              out << "\n  ],\n";

              out << "  \"Start\": \"" << currentCFG->getStartSymbol() << "\"\n";
              out << "}\n";
            }
          }

          // 3b) Build the DOT file, depending on user's choice
          std::string dotExport = exportDir + std::string(exportBaseName) + ".dot";
          switch(exportChoice) {
          case 0: // Grammar
            generateDotFileForGrammar(*currentCFG, dotExport);
            break;
          case 1: // Earley
            if (earleyParser) {
              generateDotFileForParserState(*earleyParser, dotExport);
            } else {
              ImGui::Text("No Earley parser loaded!");
            }
            break;
          case 2: // GLR
            if (glrParser) {
              generateDotFileForGLRParser(*glrParser, dotExport);
            } else {
              ImGui::Text("No GLR parser loaded!");
            }
            break;
          }

          // 3c) Convert DOT -> PNG
          std::string pngExport = exportDir + std::string(exportBaseName) + ".png";
          std::string cmd = "dot -Tpng -o " + pngExport + " " + dotExport;
          system(cmd.c_str());
        }

        // Close export menu after finishing
        exportMenuOpen = false;
      }

      ImGui::End();
    }


    // CFG Maker Window
    if (cfgMakerOpen) {
      ImGui::Begin("CFG Maker", &cfgMakerOpen);
      ImGui::Text("Create your CFG here.");
      ImGui::Separator();
      ImGui::Text("Start Symbol:");

      if (strlen(editorStartSymbol) > 0) {
        ImGui::BulletText("%s (Start)", editorStartSymbol);
        ImGui::SameLine();
        if (ImGui::Button("X##StartSymbol")) {
          std::string sSymbol(editorStartSymbol);
          if (editorNonTerminals.count(sSymbol)) {
            editorNonTerminals.erase(sSymbol);
          }
          editorStartSymbol[0] = '\0';
        }
        ImGui::Text("To change the start symbol, remove it first, then add a new one.");
      } else {
        static char tempStartSymbol[16] = "";
        ImGui::InputText("Enter Start Symbol", tempStartSymbol, IM_ARRAYSIZE(tempStartSymbol));
        if (ImGui::Button("Add Start Symbol")) {
          if (strlen(tempStartSymbol)>0) {
            std::string sSymbol(tempStartSymbol);
            if (sSymbol.size()==1 && editorTerminals.count(sSymbol[0])) {
              editorTerminals.erase(sSymbol[0]);
            }
            if(!editorNonTerminals.count(sSymbol)) {
              editorNonTerminals.insert(sSymbol);
            }
            strncpy(editorStartSymbol, tempStartSymbol, IM_ARRAYSIZE(editorStartSymbol));
            tempStartSymbol[0]='\0';
          }
        }
      }

      ImGui::Separator();
      ImGui::Text("Non-terminals:");
      {
        std::vector<std::string> ntemp(editorNonTerminals.begin(), editorNonTerminals.end());
        for (auto &nt : ntemp) {
          ImGui::BulletText("%s", nt.c_str());
          ImGui::SameLine();
          std::string delButton = "X##NT_" + nt;
          if (ImGui::Button(delButton.c_str())) {
            if(nt == editorStartSymbol) {
              editorStartSymbol[0] = '\0';
            }
            editorNonTerminals.erase(nt);
            break;
          }
        }
      }
      ImGui::InputText("Add Non-terminal", newNonTerminal, IM_ARRAYSIZE(newNonTerminal));
      if(ImGui::Button("Add NT")) {
        if(strlen(newNonTerminal)>0) {
          editorNonTerminals.insert(newNonTerminal);
          newNonTerminal[0]='\0';
        }
      }

      ImGui::Separator();
      ImGui::Text("Terminals:");
      {
        std::vector<char> ttemp(editorTerminals.begin(), editorTerminals.end());
        for (auto t: ttemp) {
          ImGui::BulletText("%c", t);
          ImGui::SameLine();
          std::string delButton = "X##T_";
          delButton.push_back(t);
          if(ImGui::Button(delButton.c_str())) {
            editorTerminals.erase(t);
            break;
          }
        }
      }
      ImGui::InputText("Add Terminal", newTerminal, IM_ARRAYSIZE(newTerminal));
      if(ImGui::Button("Add T")) {
        // skip if same as NT
        for (const auto &nt : editorNonTerminals) {
          if(std::string(newTerminal)==nt) {
            newTerminal[0]='\0';
            break;
          }
        }
        if(strlen(newTerminal)==1) {
          char t=newTerminal[0];
          if(std::string(1,t) == editorStartSymbol) {
            ImGui::TextColored(ImVec4(1,0,0,1), "Cannot add start symbol as terminal!");
          } else {
            editorTerminals.insert(t);
            newTerminal[0]='\0';
          }
        }
      }

      ImGui::Separator();
      ImGui::Text("Productions:");
      {
        struct ProdItem { std::string head; std::string body; };
        std::vector<ProdItem> allProds;
        for (auto &rule : editorProductions) {
          for (auto &bd : rule.second) {
            allProds.push_back({rule.first, bd});
          }
        }
        bool removedOne=false;
        ProdItem toRemove;
        for(auto &p: allProds) {
          ImGui::BulletText("%s -> %s", p.head.c_str(), p.body.c_str());
          ImGui::SameLine();
          std::string delBut = "X##P_"+p.head+"_"+p.body;
          if(ImGui::Button(delBut.c_str())) {
            removedOne=true; toRemove=p; break;
          }
        }
        if(removedOne) {
          auto &vec = editorProductions[toRemove.head];
          vec.erase(std::remove(vec.begin(), vec.end(), toRemove.body), vec.end());
          if(vec.empty()) {
            editorProductions.erase(toRemove.head);
          }
        }
      }

      ImGui::InputText("Prod Head", prodHead, IM_ARRAYSIZE(prodHead));
      ImGui::InputText("Prod Body (symbols concatenated)", prodBody, IM_ARRAYSIZE(prodBody));
      if(ImGui::Button("Add Production")) {
        if(strlen(prodHead)>0 && editorNonTerminals.count(prodHead)>0) {
          editorProductions[prodHead].push_back(std::string(prodBody));
          prodHead[0]='\0';
          prodBody[0]='\0';
        }
      }

      ImGui::Separator();
      if(ImGui::Button("Save as JSON |CreatedJson.json| ")) {
        std::string savePath = grammarsDir + std::string("CreatedJson.json");
        saveEditorCFGToJSON(savePath);
        try {
          currentCFG = std::make_unique<CFG>(savePath);
          earleyParser = std::make_unique<EarleyParser>(*currentCFG);
          glrParser = std::make_unique<GLRParser>(*currentCFG);
          updateGraphVisualization();
          refreshAvailableGrammars();
        }catch(...){}
      }
      ImGui::End();
    }

    // Main parsing controls
    ImGui::Begin("Main Controls");
    ImGui::InputText("CFG File Path", grammarPath, IM_ARRAYSIZE(grammarPath));
    if(ImGui::Button("Load Grammar")) {
      try {
        currentCFG = std::make_unique<CFG>(grammarPath);
        earleyParser = std::make_unique<EarleyParser>(*currentCFG);
        glrParser = std::make_unique<GLRParser>(*currentCFG);
        parseResultEarley = "Grammar Loaded!";
        parseResultGLR = "Grammar Loaded!";
        stepByStepEarley=false;
        earleyFinished=false;
        stepByStepGLR=false;
        glrFinished=false;
        updateGraphVisualization();
      }catch(const std::exception &e) {
        parseResultEarley = "Failed to load grammar: ";
        parseResultEarley += e.what();
        parseResultGLR = parseResultEarley;
      }

    }

    ImGui::InputText("Input String", inputString, IM_ARRAYSIZE(inputString));

    // Earley
    if(ImGui::Button("Earley Parse (Full)")) {
      if(earleyParser) {
        bool res = earleyParser->parse(inputString);
        parseResultEarley = res?"Accepted":"Rejected";
        updateGraphVisualization();
      }
    }
    ImGui::SameLine();
    if(ImGui::Button("Earley Step-by-Step")) {
      if(earleyParser) {
        earleyParser->reset(inputString);
        stepByStepEarley=true;
        earleyFinished=false;
        updateGraphVisualization();
      }
    }
    if(stepByStepEarley && !earleyFinished) {
      if(ImGui::Button("Next Step (Earley)")) {
        bool cont = earleyParser->nextStep();
        if(!cont) {
          earleyFinished=true;
          parseResultEarley = earleyParser->isAccepted()?"Accepted":"Rejected";
        }
        updateGraphVisualization();
      }
    }

    ImGui::Separator();

    // GLR
    if(ImGui::Button("GLR Parse (Full)")) {
      if(glrParser) {
        glrParser->reset(inputString);
        while(!glrParser->isDone()) {
          glrParser->nextStep();
        }
        parseResultGLR = glrParser->isAccepted()?"Accepted":"Rejected";
        updateGraphVisualization();
      }
    }
    ImGui::SameLine();
    if(ImGui::Button("GLR Step-by-Step")) {
      if(glrParser) {
        glrParser->reset(inputString);
        stepByStepGLR=true;
        glrFinished=false;
        updateGraphVisualization();
      }
    }
    if(stepByStepGLR && !glrFinished) {
      if(ImGui::Button("Next Step (GLR)")) {
        bool cont = glrParser->nextStep();
        if(!cont){
          glrFinished=true;
          parseResultGLR = glrParser->isAccepted()?"Accepted":"Rejected";
        }
        updateGraphVisualization();
      }
    }

    ImGui::Separator();
    ImGui::Text("Earley result: %s", parseResultEarley.c_str());
    ImGui::Text("GLR result:   %s", parseResultGLR.c_str());

    if(ImGui::Button("Show Graph")) {
      showGraphWindow=true;
    }
    ImGui::SameLine();
    if(ImGui::Button("Show Legend")) {
      showLegendWindow=true;
    }

    ImGui::End();

    // Graph Window
    if(showGraphWindow) {
      ImGui::Begin("Graph Visualization", &showGraphWindow);
      if(graphTexture) {
        ImGui::Text("Graph (width=%d, height=%d):", graphTexWidth, graphTexHeight);
        ImGui::Image((ImTextureID)(intptr_t)graphTexture, ImVec2((float)graphTexWidth, (float)graphTexHeight));
      } else {
        ImGui::Text("No graph available.");
      }
      ImGui::End();
    }

    // Legend
    if(showLegendWindow) {
      ImGui::Begin("Legend", &showLegendWindow);
      ImGui::Text("Earley Parser Color Legend:");
      ImGui::BulletText("Red:    Predict (dotPos=0)");
      ImGui::BulletText("Yellow: Scan    (0 < dotPos < len)");
      ImGui::BulletText("Green:  Complete(dotPos= len)");
      ImGui::BulletText("Accept: doublecircle, green node");
      ImGui::BulletText("Reject: grey octagon node");
      ImGui::End();
    }

    // Render
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0,0,display_w,display_h);
    glClearColor(0.45f,0.55f,0.60f,1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  // shutdown
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
