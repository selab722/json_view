#ifndef AVE_GUI_OPENGL__WINDOW_H
#define AVE_GUI_OPENGL__WINDOW_H


#include <string>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include "imgui.h"
#include "ave/gui/save_config.h"
#include "ave/core/engine.h"


namespace ave {


class OpenglGui : public RunnerContainer {
public:
    OpenglGui() {}

    bool init();

    /**
     * @param       fps Pass 1 if it's continues, it will call glfwPollEvents().
     *                  Pass 0 if it's not continues, it will call glfwWaitEvents().
     *                  If you don't know what that mean, pass 1.
     * @return      true if should continue
     */
    virtual bool loop(int fps);

    virtual ~OpenglGui();
};

class GlWindow : public RunnerContainer {
private:
    GLFWwindow* window_ = nullptr;
    ImGuiContext* imgui_context_ = nullptr;
    bool show_demo_window_ = false;
public:
    SaveConfig save_config_;
    std::string title_;


    GlWindow(std::string save_file="") : save_config_(save_file) {}

    /**
     * @brief       Initialize glfw and imgui
     * @details     Example of calling:
     *              const char* font_location[] = {"a.otf", "res/a.otf", ""};
     *              gui->init(font_location, 22);  // or you can do this:
     *              gui->init(nullptr, 0);
     * @param       font_file The file name of font file. You can put multiple locations here,
     *              in case you can't find it, but remember to end with "". If you simply pass nullptr,
     *              default font will be used.
     *              The function will search them one by one.
     * @param       font_size Font size. Will be ignored if no font_file.
     * @return      true if success
     */
    bool init(const char* font_file[], const int font_size);

    /**
     * @param       int parameter not used
     */
    virtual bool loop(int);


    GlWindow(const GlWindow&) = delete;
    GlWindow(GlWindow&&) = default;
    GlWindow& operator=(const GlWindow&) = delete;
    GlWindow& operator=(GlWindow&&) = delete;

    virtual ~GlWindow();

};


}  // namespace ave

#endif  // #ifndef AVE_GUI_OPENGL__WINDOW_H
