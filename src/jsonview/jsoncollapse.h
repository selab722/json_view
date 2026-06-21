#ifndef JSONVIEW_JSONCOLLAPSE_H
#define JSONVIEW_JSONCOLLAPSE_H

#include <unordered_map>

#include "json/json.h"

namespace jsonview {



class JsonCollapse : public json::Json {
public:
    std::unordered_map<int, bool> buttons;

    // this constructor will consume the Json object.
    JsonCollapse( json::Json&& j ) : Json(j.root.release()) {
        // root.reset(j.root.release());
        set_map(root.get());
        if( root.get()->type == json::JsonType::Object || root.get()->type == json::JsonType::Array )
            buttons[root.get()->id] = true;
    }

    JsonCollapse(JsonCollapse&&) = default;
    JsonCollapse& operator=(JsonCollapse&&) = default;

    JsonCollapse(const JsonCollapse&) = delete;
    JsonCollapse& operator=(const JsonCollapse&) = delete;

    ~JsonCollapse() = default;

private:
    void set_map( json::JsonNode* node ) {

        // if( typeid(*node) == typeid(json::JsonNodeObject) ) {
        if( node->type == json::JsonType::Object ) {
            buttons.emplace(node->id, false);

            for( std::pair<std::string, json::JsonNode*>& p : ((json::JsonNodeObject*)node)->pairs )
                set_map(p.second);

        }
        // else if( typeid(*node) == typeid(json::JsonNodeArray) ) {
        else if( node->type == json::JsonType::Array ) {
            buttons.emplace(node->id, false);

            for (json::JsonNode*& child : ((json::JsonNodeArray*)node)->array)
                set_map(child);
        }
    }
};

}

#endif
