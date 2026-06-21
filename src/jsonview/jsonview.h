#ifndef JSONVIEW_JSONVIEW_H
#define JSONVIEW_JSONVIEW_H


#include "ave/gui/imgui_window.h"
#include "json/json.h"
#include "jsonview/jsoncollapse.h"


namespace jsonview {

class JsonViewer : public ave::ImGuiWindowRunner {
public:
    std::vector<std::pair<std::string, JsonCollapse>> jsonIds;

    // constructor will take control of this json.
    // after that this argument json will be null.
    JsonViewer(std::string save_file, bool fps = false) : ImGuiWindowRunner(save_file, fps), selected_json(nullptr) {}

    void add_json( const std::string& name, json::Json json ) {
        // std::cout<<json<<std::endl;
        jsonIds.emplace_back(name, std::move(json));
    }

    // return if continue
    virtual bool dowork( void* );

    // cannot copy, but ok to move
    JsonViewer(const JsonViewer&) = delete;
    JsonViewer(JsonViewer&&) = default;
    JsonViewer& operator=(const JsonViewer&) = delete;
    JsonViewer& operator=(JsonViewer&&) = delete;

    // virtual ~JsonViewer() = default;

private:
    JsonCollapse* selected_json;
    static void draw_json( JsonCollapse&, const json::JsonNode*, bool should_indent );
};

}

#endif