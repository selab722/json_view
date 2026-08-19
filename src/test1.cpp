#include <iostream>
#include <filesystem>
#include "ave/gui/window/opengl_window.h"
#include "jsonview/jsonview.h"

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::unique_ptr;
using std::vector;
using std::stringstream;
using namespace ave;
using namespace json;
using namespace jsonview;
namespace fs = std::filesystem;


Json get_json( const char* json_file ) {
    std::ifstream fin(json_file);
    char c;
    JsonParser parser;
    int result = 0;
    if (fin.is_open())
        while( fin.get(c) ) {
            result = parser.parse(c);
        }
    else{
        parser.parse('{');
        parser.parse('}');
    }
    parser.parse(0);
    return parser.get();
}

Json empty_json() {
    JsonParser parser;
    parser.parse(0);
    return parser.get();
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

        unique_ptr<JsonViewer> jsonview = std::make_unique<JsonViewer>();
        jsonview->add_json("j1", get_json("res/j1.json"));
        jsonview->add_json("j2", get_json("res/j2.json"));
        jsonview->add_json("j3", empty_json());

        window->set_runner(std::move(jsonview));
        framework->set_runner(std::move(window));
    }

    while( framework->loop(1) );


    return 0;
}
