#ifndef AVE_GUI_IMGUI__WINDOW_H
#define AVE_GUI_IMGUI__WINDOW_H

#include "ave/gui/opengl_window.h"

namespace ave {

class ImGuiWindowRunner : public GlWindowRunner {
public:

    const static int FONT_SIZE = 22;

    ImGuiWindowRunner(std::string save_file="", bool fps = false ) : GlWindowRunner(save_file, fps) {}

    // Return if succees
    // Pass title of windows and location of font like this:
    // char* init_args[] = { "Window", "D:/font1.otf", "D:/font2.otf", "" };
    // runner->init(init_args);
    // First is window title. Follows some of the possible font files location.
    // Program will look from left to right until found font file.
    // End array with empty string "".
    // If you don't pass anything, pass null.
    // If you don't specify font file location, do:
    // char* init_args[] = { "Window", "" };
    virtual bool init( void* );

    virtual bool loop_begin( void* );
    virtual void loop_end( void* );

    ImGuiWindowRunner(const ImGuiWindowRunner&) = delete;
    ImGuiWindowRunner(ImGuiWindowRunner&&) = default;
    ImGuiWindowRunner& operator=(const ImGuiWindowRunner&) = delete;
    ImGuiWindowRunner& operator=(ImGuiWindowRunner&&) = delete;

};

}


#ifdef AVE_GUI_IMGUI__WINDOW_H_IMPLEMENTATION


#include <iostream>
#include <filesystem>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::stringstream;
namespace fs = std::filesystem;


namespace ave {


bool ImGuiWindowRunner::init( void* init_args ) {

    if( !GlWindowRunner::init(init_args) )
        return false;


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();

    char* font_file = nullptr;
    if( init_args )
        for( char** i = (static_cast<char**>(init_args)+1); i && (*i)[0]; i++ ) try {
            if( fs::exists(*i) ) {
                font_file = *i;
                break;
            }
        } catch (...) {}

    if( font_file )
        io.Fonts->AddFontFromFileTTF(font_file, FONT_SIZE);
    else
        io.Fonts->AddFontDefault();

    // io.Fonts->AddFontDefault();
    // if( init_args ) {
    //     ImFontConfig config;
    //     config.MergeMode = true;
    //     io.Fonts->AddFontFromFileTTF(static_cast<char**>(init_args)[1], font_size, &config);
    // }


    {
        float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());  //TODO
        style.ScaleAllSizes(main_scale);
        style.FontScaleDpi = main_scale;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    io.IniFilename = nullptr;

    if( font_file )
        style.FontSizeBase = FONT_SIZE;

    return true;
}


bool ImGuiWindowRunner::loop_begin( void* pointer ) {
    if( !GlWindowRunner::loop_begin( pointer ) )
        return false;

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one or the other.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.8f, 0.9f, 0.8f, 1.0f));
    ImGui::Begin("Fullscreen window", nullptr, flags);
    ImGui::PopStyleColor();

    return true;
}


void ImGuiWindowRunner::loop_end( void* pointer ) {
    ImGui::End();

    if (ImGui::IsKeyPressed(ImGuiKey_Tab))
        show_demo_window = !show_demo_window;

    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    GlWindowRunner::loop_end( pointer );
}


}


#endif  // #ifdef AVE_GUI_IMGUI__WINDOW_H_IMPLEMENTATION
#endif  // #ifndef AVE_GUI_IMGUI__WINDOW_H