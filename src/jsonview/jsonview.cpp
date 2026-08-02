#include <iostream>

#include "json/utils.h"
#include "jsonview/jsonview.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using std::cerr;
using std::cout;
using std::endl;
using std::pair;
using std::string;
using std::vector;
using std::stringstream;
using namespace json;
using namespace jsonview;


namespace {

void pop_up_window( bool pop_up ) {
    if( pop_up )
        ImGui::OpenPopup("show_clip");


    if (ImGui::BeginPopup("show_clip")) {
        ImGui::Text("From clipboard:");
        const char* clipboard_text = ImGui::GetClipboardText();
        ImGui::Text("%s", clipboard_text);
        ImGui::EndPopup();
    }
}


}




bool JsonViewer::loop( int ) {

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

    ImGui::AlignTextToFramePadding();
    ImGui::Text("Copy all");
    ImGui::SameLine();
    if(ImGui::SmallButton("C") && selected_json ) {
        std::stringstream ss;
        selected_json->root->print(ss,0);
        ImGui::SetClipboardText(ss.str().c_str());
    }

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Show all");
    ImGui::SameLine();
    pop_up_window(ImGui::SmallButton("S"));

    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

    if (ImGui::BeginTabBar("MyTabBar", ImGuiTabBarFlags_None))
    {
        for( pair<string, JsonCollapse>& p : jsonIds )
            if( ImGui::BeginTabItem(p.first.c_str()) ) {
                selected_json = &(p.second);
                if (ImGui::BeginChild("ScrollableArea",
                        ImVec2(0, 0),
                        false,
                        ImGuiWindowFlags_HorizontalScrollbar )) {
                    draw_json(p.second, p.second.root.get(), false );
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

        ImGui::EndTabBar();
    }

    ImGui::End();
    return true;
}


namespace {

bool expandButton( int id, bool& expand, int type ) {
    const static char* token_one[] = { "{", "[", "(" };
    const static char* token_two[] = { "{}", "[]", "()" };

    ImGui::PushID(id);
    expand = (expand != ImGui::Button(expand ? "v###but" : ">###but"));
    ImGui::SameLine();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", expand ? token_one[type] : token_two[type]);

    ImGui::PopID();
    return expand;
}


void textCopy( int id, const char* text, const char* password ) {
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", text);
    ImGui::PushID(id);
    ImGui::SameLine();
    if( ImGui::SmallButton("C") )
        ImGui::SetClipboardText(password);
    ImGui::PopID();
}


void draw_json_pass( JsonCollapse& jsonId, const json::JsonNode* node ) {
    if( !node || node->type != JsonType::Object ) {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Password: Error1");
        return;
    }
    const int id = node->id;
    if( !expandButton( id, jsonId.buttons[id], 2) )
        return;

    ImGui::Indent();
    for( int i = 0; i < ((JsonNodeObject*)node)->pairs.size(); i ++ ) {
        const pair<string, JsonNode*>& p = ((JsonNodeObject*)node)->pairs[i];
        if( !p.second ) {
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Password: Error2");
            return;
        }
        const int id2 = p.second->id;

        if( typeid(*p.second) == typeid(JsonNodeSingle) ) {
            string value = ((JsonNodeSingle*)p.second)->value;
            if( p.second->type == JsonType::Error )
                value = "error: "+value;
            textCopy(id2, p.first.c_str(), value.c_str());
        } else
            switch (p.second->type ) {
                case JsonType::True:
                    textCopy(id2, p.first.c_str(), "true");
                    break;
                case JsonType::False:
                    textCopy(id2, p.first.c_str(), "false");
                    break;
                case JsonType::Null:
                    textCopy(id2, p.first.c_str(), "null");
                    break;
                default:
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Password: Error3");
                    break;
            }
    }
    ImGui::Unindent();
    ImGui::AlignTextToFramePadding();
    ImGui::Text(")");

}

}

void JsonViewer::draw_json( JsonCollapse& jsonId, const json::JsonNode* node, bool should_indent ) {
    const int id = node->id;
    if( typeid(*node) == typeid(JsonNodeObject) ) {
        if( ((JsonNodeObject*)node)->pairs.size()==1 && ((JsonNodeObject*)node)->pairs[0].first == "Password:" ) {
            draw_json_pass( jsonId, ((JsonNodeObject*)node)->pairs[0].second );
            return;
        }
        if( !expandButton( id, jsonId.buttons[id], 0) )
            return;
        if( should_indent )
            ImGui::Indent();
        for( int i = 0; i < ((JsonNodeObject*)node)->pairs.size(); i ++ ) {
            const pair<string, JsonNode*>& p = ((JsonNodeObject*)node)->pairs[i];
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s :", p.first.c_str());
            JsonType child_type = p.second->type;
            // if( child_type!=JsonType::Object && child_type!=JsonType::Array ) {
            ImGui::SameLine();
            draw_json( jsonId, p.second, true);

        }
        if( should_indent )
            ImGui::Unindent();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("}");

    } else if( typeid(*node) == typeid(JsonNodeArray) ) {
        if( !expandButton( id, jsonId.buttons[id], 1 ) )
            return;
        if( should_indent )
            ImGui::Indent();
        for( JsonNode* j : ((JsonNodeArray*)node)->array )
            draw_json(jsonId, j, true);
        if( should_indent )
            ImGui::Unindent();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("]");
    } else if( typeid(*node) == typeid(JsonNodeSingle) ) {
        string value = ((JsonNodeSingle*)node)->value;
        if( node->type == JsonType::Error )
            value = "error: "+value;
        ImGui::AlignTextToFramePadding();
        ImGui::Text("%s", value.c_str());
    } else switch (node->type ) {
        case JsonType::True:
            ImGui::AlignTextToFramePadding();
            ImGui::Text("true");
            break;
        case JsonType::False:
            ImGui::AlignTextToFramePadding();
            ImGui::Text("false");
            break;
        case JsonType::Null:
            ImGui::AlignTextToFramePadding();
            ImGui::Text("null");
            break;
        default:
            cerr<<"unknown type: "<<endl<<typeid(*node).name()<<"  "<<(node->type)<<endl;
    }
}
