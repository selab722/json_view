#ifndef AVE_GUI_OPENGL__WINDOW_H
#define AVE_GUI_OPENGL__WINDOW_H


#include <string>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "ave/gui/save_config.h"
#include "ave/core/engine.h"

namespace ave {

class GlWindowRunner : public GameRunner {
public:
    GLFWwindow* window;
    bool show_demo_window = false;
    SaveConfig save_config;
    const bool is_fps;


    GlWindowRunner(std::string save_file="", bool fps = false ) : save_config(save_file), is_fps(fps) {}

    // Return if succees.
    // Pass title of windows as argument like:
    // char* init_args[] = { "Window" };
    // runner->init(init_args);
    // If you don't pass init_args, pass null.
    virtual bool init( void* );

    // return if continue
    virtual bool loop( void* ptr ) {
        if( !loop_begin(ptr) )
            return false;
        bool should_continue = dowork(ptr);
        loop_end(ptr);
        return should_continue;
    }

    virtual bool loop_begin( void* );

    virtual bool dowork( void* ) {
        return true;
    }

    virtual void loop_end( void* );




    GlWindowRunner(const GlWindowRunner&) = delete;
    GlWindowRunner(GlWindowRunner&&) = default;
    GlWindowRunner& operator=(const GlWindowRunner&) = delete;
    GlWindowRunner& operator=(GlWindowRunner&&) = delete;

    virtual ~GlWindowRunner();

};

}



#ifdef AVE_GUI_OPENGL__WINDOW_H_IMPLEMENTATION


#include <iostream>

#include <glad/gl.h>

using std::cerr;
using std::cout;
using std::endl;


namespace ave {


bool GlWindowRunner::init( void* init_args ) {

    glfwSetErrorCallback([](int error, const char* description) {
        cerr<<"Error: "<<description<<endl;
    });

    if (!glfwInit())
        return false;


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


    {
        constexpr int MIN = 0x80000000;
        int xpos = MIN;
        int ypos = MIN;
        int width = 800;
        int height = 800;
        save_config.read(xpos, ypos, width, height);

        window = glfwCreateWindow(width, height, init_args ? static_cast<char**>(init_args)[0] : "GL Window", nullptr, nullptr);
        if( xpos != MIN )
            glfwSetWindowPos(window, xpos, ypos);
    }

    if (!window) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        glViewport(0, 0, width, height);
    });
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);
    });
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        glfwTerminate();
        return false;
    }
    glEnable(GL_DEPTH_TEST);


    return true;
}


bool GlWindowRunner::loop_begin( void* ) {
    if( glfwWindowShouldClose(window) )
        return false;

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return true;
}

void GlWindowRunner::loop_end( void* ) {

    glfwSwapBuffers(window);
    if( is_fps )
        glfwPollEvents();
    else
        glfwWaitEvents();
}


GlWindowRunner::~GlWindowRunner() {
    if (window) {
        int width, height, xpos, ypos;
        glfwGetWindowSize(window, &width, &height);
        glfwGetWindowPos(window, &xpos, &ypos);
        
        save_config.write(xpos, ypos, width, height);
    }
    glfwTerminate();
}


}  // namespace ave

#endif  // #ifdef AVE_GUI_OPENGL__WINDOW_H_IMPLEMENTATION
#endif  // #ifndef AVE_GUI_OPENGL__WINDOW_H