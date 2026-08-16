#include <iostream>
#include <filesystem>
#include "ave/gui/window/opengl_window.h"
#include "ave/gui/chooser/file_chooser.h"

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::unique_ptr;
using std::vector;
using std::stringstream;
using namespace ave;
namespace fs = std::filesystem;


class MyWindow : public Runner {
public:
    FileChooser fc;

    virtual bool loop(int) {
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

        if(ImGui::Button("Choose") || fc.is_open())
            fc.show();
        ImGui::SameLine();
        if( fc.get_selected_file().empty() )
            ImGui::Text("Unselected");
        else
            ImGui::Text("%s", fc.get_selected_file().string().c_str());


        ImGui::End();
        return true;
    }
};


string combine_path(const char* path, const char* file) {
    try {
        fs::path exe_path = fs::absolute(path);
        return (exe_path.parent_path() / file).string();
    } catch (const std::exception& e) {
        std::cerr << "bin path error: " << e.what() << endl;
        return "";
    }
}


int main( int argc, char** argv ) {

    unique_ptr<OpenglGui> framework = std::make_unique<OpenglGui>();

    if( !framework->init() ) {
        cerr<<"GLfw Init Fail"<<endl;
        return -1;
    }

    {
        string font_path = argc ? combine_path(argv[0], "SourceHanSansSC-Regular.otf") : "SourceHanSansSC-Regular.otf";
        const char* init_args[] = {
            font_path.c_str(),
            "res/SourceHanSansSC-Regular.otf",
            "res/fonts/SourceHanSansSC-Regular.otf",
            "SourceHanSansSC-Regular.otf",
            ""
        };
        unique_ptr<GlWindow> window =
                std::make_unique<GlWindow>(argc ? combine_path(argv[0], "file.ini") : "");
        window->title_ = "Test";
        if( !window->init(init_args, 22) )
            return -1;

        unique_ptr<MyWindow> my_window = std::make_unique<MyWindow>();

        window->set_runner(std::move(my_window));
        framework->set_runner(std::move(window));
    }

    while( framework->loop(1) );


    return 0;
}
