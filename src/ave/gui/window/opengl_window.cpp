
#include <iostream>
#include <filesystem>
#include <limits>
#include <glad/gl.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ave/gui/window/opengl_window.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
namespace fs = std::filesystem;


namespace ave {


bool OpenglGui::init() {
    glfwSetErrorCallback([](int error, const char* description) {
        cerr<<"Glfw Error: "<<description<<endl;
    });

    if (!glfwInit())
        return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    return true;
}

bool OpenglGui::loop(int fps) {
    if( fps )
        glfwPollEvents();
    else
        glfwWaitEvents();
    Runner* runner = get_runner();
    return runner && runner->loop(0);
}

OpenglGui::~OpenglGui() {
    set_runner(nullptr);
    glfwTerminate();
}


bool GlWindow::init(const char* font_file[], const int font_size) {
    {
        const int MIN = std::numeric_limits<int>::min();
        int xpos = MIN;
        int ypos = MIN;
        int width = 800;
        int height = 800;
        save_config_.read(xpos, ypos, width, height);

        window_ = glfwCreateWindow(width, height, title_.empty() ? "GL Window" : title_.c_str(), nullptr, nullptr);
        if( xpos != MIN )
            glfwSetWindowPos(window_, xpos, ypos);
    }

    if (!window_)
        return false;

    glfwMakeContextCurrent(window_);

    if (!gladLoadGL(glfwGetProcAddress)) {
        cerr<<"Failed to initialize GLAD"<<endl;
        glfwDestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    glfwSetFramebufferSizeCallback(window_, [](GLFWwindow *window, int width, int height) {
        glViewport(0, 0, width, height);
    });
    glfwSetKeyCallback(window_, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    });  // TODO
    glfwSwapInterval(1);

    glEnable(GL_DEPTH_TEST);


    IMGUI_CHECKVERSION();
    imgui_context_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context_);
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();

    if (font_file != nullptr)
        for (int i = 0; font_file[i] != nullptr && font_file[i][0] != '\0'; ++i) {
            const char* path = font_file[i];
            if (fs::exists(path)) {
                io.Fonts->AddFontFromFileTTF(path, font_size);
                style.FontSizeBase = font_size;
                break;
            }
        }

    float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());  //TODO
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    io.IniFilename = nullptr;
    return true;
}

bool GlWindow::loop(int) {
    if( glfwWindowShouldClose(window_) )
        return false;

    glfwMakeContextCurrent(window_);
    ImGui::SetCurrentContext(imgui_context_);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    Runner* runner = get_runner();
    bool result = !runner || runner->loop(0);

    if (ImGui::IsKeyPressed(ImGuiKey_Tab))
        show_demo_window_ = !show_demo_window_;

    if (show_demo_window_)
        ImGui::ShowDemoWindow(&show_demo_window_);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);
    return result;
}

GlWindow::~GlWindow() {
    set_runner(nullptr);
    if (!window_)
        return;

    int width, height, xpos, ypos;
    glfwGetWindowSize(window_, &width, &height);
    glfwGetWindowPos(window_, &xpos, &ypos);
    
    save_config_.write(xpos, ypos, width, height);

    glfwMakeContextCurrent(window_);
    ImGui::SetCurrentContext(imgui_context_);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(imgui_context_);

    imgui_context_ = nullptr;
    glfwDestroyWindow(window_);
    window_ = nullptr;
}


}  // namespace ave
