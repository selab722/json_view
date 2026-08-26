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

        ImGui::Begin("Fullscreen window", nullptr, flags);

        if( fc.is_open() ) {
            // cout<<"mul: "<<fc.is_multiple_selection()<<endl;
            fc.show();
        }

        if( ImGui::Button("Single File") ) {
            fc.reset(true);
            fc.set_selection_mode(FileChooser::SelectionMode::FilesOnly);
            fc.set_multiple_selection(false);
            fc.show();
        }

        if( ImGui::Button("Multiple File") ) {
            fc.reset(true);
            fc.set_selection_mode(FileChooser::SelectionMode::FilesOnly);
            fc.set_multiple_selection(true);
            fc.show();
        }

        if( ImGui::Button("Single Dir") ) {
            fc.reset(true);
            fc.set_selection_mode(FileChooser::SelectionMode::DirectoriesOnly);
            fc.set_multiple_selection(false);
            fc.show();
        }

        if( ImGui::Button("Multiple Dir") ) {
            fc.reset(true);
            fc.set_selection_mode(FileChooser::SelectionMode::DirectoriesOnly);
            fc.set_multiple_selection(true);
            fc.show();
        }

        if( fc.get_selected_file().empty() )
            ImGui::Text("Unselected");
        else
            ImGui::Text("%s", fc.get_selected_file().string().c_str());

        vector<fs::path> selected = fc.get_selected_files();
        for( int i = 0; i < selected.size(); i ++ )
            ImGui::Text("%d %s", i, selected[i].u8string().c_str());

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
        string font_path = (fs::path(argv[0]).parent_path()/"SourceHanSansSC-Regular.otf").u8string();
        const char* init_args[] = {
            font_path.c_str(),
            "res/SourceHanSansSC-Regular.otf",
            "res/fonts/SourceHanSansSC-Regular.otf",
            "SourceHanSansSC-Regular.otf",
            ""
        };
        unique_ptr<GlWindow> window =
                std::make_unique<GlWindow>((fs::path(argv[0]).parent_path()/"file.ini").u8string());
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
