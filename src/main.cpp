#include <iostream>
#include <filesystem>

#include "jsonview/jsonview.h"

using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::vector;
using std::stringstream;
using namespace json;
using namespace jsonview;
namespace fs = std::filesystem;


//g++ -static -static-libgcc -static-libstdc++ -Ibuild/install/include src/main.cpp build/install/lib/*.a -lws2_32 -lgdi32 -mwindows -o main.exe



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

    std::unique_ptr<JsonViewer> runner = std::make_unique<JsonViewer>(argc ? combine_path(argv[0], "init.txt") : "");

    string font_path = argc ? combine_path(argv[0], "SourceHanSansSC-Regular.otf") : "SourceHanSansSC-Regular.otf";

    const char* init_args[] = { "PassView",
        font_path.c_str(),
        "res/SourceHanSansSC-Regular.otf",
        "res/fonts/SourceHanSansSC-Regular.otf",
        "SourceHanSansSC-Regular.otf",
        ""
    };

    runner->add_json("j1", get_json("res/j1.json"));
    runner->add_json("j2", get_json("res/j2.json"));
    runner->add_json("j3", empty_json());

    if( !runner->init(init_args) ) {
        cerr<<"init error"<<endl;
        return -1;
    }

    while( runner->loop(nullptr) );

    return 0;
}
