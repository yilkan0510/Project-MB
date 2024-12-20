#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

#include "logic/CFG.h"
#include "logic/EarleyParser.h"
#include "logic/GLRParser.h"

// Include Dear ImGui
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

// Include GLFW
#include <GL/glew.h>    // Initialize with glewInit()
#include <GLFW/glfw3.h>



// Forward declarations
static void glfw_error_callback(int error, const char* description) {
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Setup Dear ImGui context with docking
void SetupImGuiDockspace() {
  // When docking is enabled, we can create a DockSpace
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);

  ImGuiWindowFlags window_flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("DockSpace Demo", nullptr, window_flags);
  ImGui::PopStyleVar(2);

  ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f,0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
  ImGui::End();
}

static bool stepByStepEarley = false;
static bool earleyFinished = false;


int main(int, char**)
{
  // Setup GLFW
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW\n");
    return 1;
  }

  // GL 3.0 + GLSL 130
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,0);

  // Create window with graphics context
  GLFWwindow* window = glfwCreateWindow(1280, 720, "CFG Visualization", NULL, NULL);
  if (window == NULL) {
    fprintf(stderr, "Failed to create GLFW window\n");
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); // vsync

  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    return 1;
  }

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();(void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  // Variables for the UI
  static char grammarFile[128] = "../src/JSON/CFG4.json"; // default grammar file
  static char inputString[128] = "";
  static std::string parseResultEarley = "";
  static std::string parseResultGLR = "";
  bool loadGrammarRequested = false;

  // Once loaded
  std::unique_ptr<CFG> currentCFG;
  std::unique_ptr<EarleyParser> earleyParser;
  std::unique_ptr<GLRParser> glrParser;

  while (!glfwWindowShouldClose(window))
  {
    // Poll and handle events
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Create a docking space
    {
      ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;
      ImGui::Begin("Main Menu", nullptr, window_flags);

      if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
          ImGui::MenuItem("Exit", NULL, false, false);
          ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
      }




      ImGui::InputText("Grammar File", grammarFile, IM_ARRAYSIZE(grammarFile));
      if (ImGui::Button("Load Grammar")) {
        loadGrammarRequested = true;
      }

      ImGui::Separator();
      ImGui::InputText("Input String", inputString, IM_ARRAYSIZE(inputString));

      // Below the normal parse buttons:
      if (ImGui::Button("Earley Step-by-Step")) {
        if (earleyParser) {
          earleyParser->reset(inputString);
          stepByStepEarley = true;
          earleyFinished = false;
        }
      }
      ImGui::SameLine();
      if (stepByStepEarley && !earleyFinished) {
        if (ImGui::Button("Next Step (Earley)")) {
          bool cont = earleyParser->nextStep();
          if (!cont) {
            earleyFinished = true;
          }
        }
      }

      // Display Earley chart if step-by-step mode is active
      if (stepByStepEarley) {
        ImGui::Separator();
        ImGui::Text("Earley Position: %zu", earleyParser->getCurrentPos());
        const auto &chartSets = earleyParser->getChart();
        for (size_t i=0; i<chartSets.size(); i++) {
          ImGui::Text("Chart[%zu]:", i);
          for (auto &it : chartSets[i]) {
            std::string dotBody = it.body;
            dotBody.insert(it.dotPos, "•");
            ImGui::BulletText("%s -> %s (start=%zu)", it.head.c_str(), dotBody.c_str(), it.startIdx);
          }
        }
        if (earleyParser->isDone()) {
          ImGui::Text("Earley Done. Accepted: %s", earleyParser->isAccepted() ? "Yes" : "No");
        }
      }

      if (ImGui::Button("Parse with Earley")) {
        if (earleyParser && glrParser) {
          bool result = earleyParser->parse(inputString);
          parseResultEarley = result ? "Accepted" : "Rejected";
        } else {
          parseResultEarley = "No grammar loaded!";
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Parse with GLR")) {
        if (earleyParser && glrParser) {
          bool result = glrParser->parse(inputString);
          parseResultGLR = result ? "Accepted" : "Rejected";
        } else {
          parseResultGLR = "No grammar loaded!";
        }
      }

      ImGui::Separator();
      ImGui::Text("Earley Result: %s", parseResultEarley.c_str());
      ImGui::Text("GLR Result:    %s", parseResultGLR.c_str());

      ImGui::End(); // end main menu window
    }

    // Dockspace background
    SetupImGuiDockspace();

    // If we requested to load a new grammar
    if (loadGrammarRequested) {
      loadGrammarRequested = false;
      try {
        currentCFG = std::make_unique<CFG>(grammarFile);
        earleyParser = std::make_unique<EarleyParser>(*currentCFG);
        glrParser = std::make_unique<GLRParser>(*currentCFG);
        parseResultEarley = "Grammar Loaded!";
        parseResultGLR = "Grammar Loaded!";
        stepByStepEarley = false;
        earleyFinished = false;
      } catch(...) {
        parseResultEarley = "Failed to load grammar";
        parseResultGLR = "Failed to load grammar";
      }
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

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
