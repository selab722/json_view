#ifndef JSON_JSON_H
#define JSON_JSON_H

#include <string>
#include <vector>
#include <utility>
#include <stack>
#include <sstream>
#include <ostream>
#include <memory>

namespace json {

enum class JsonType {
    Object,                         // use JsonNodeObject
    Array,                          // use JsonNodeArray
    String, Number, Error,          // use JsonNodeSingle
    Tmp,                            // use JsonNodeTmp
    True, False, Null               // use JsonNode
};

std::ostream& operator<<(std::ostream&, const JsonType&);

class JsonNode {
public:
    const JsonType type;
    int id;

    JsonNode(JsonType type) : type(type), id(0) {}

    virtual void print( std::ostream& os, int indent ) const {
        for( int i = 0; i < indent; i ++ )
            os<<"    ";
        switch( type )
        {
            case JsonType::True:
                os<<"true";
                break;
            case JsonType::False:
                os<<"false";
                break;
            case JsonType::Null:
                os<<"null";
                break;
            default:
                os<<"error in JsonNode::print(): "<<type<<std::endl;
        }
    }

    JsonNode(const JsonNode&) = delete;
    JsonNode(JsonNode&&) = delete;
    JsonNode& operator=(const JsonNode&) = delete;
    JsonNode& operator=(JsonNode&&) = delete;
    
    virtual ~JsonNode() = default;
};


class JsonNodeObject : public JsonNode {
public:
    std::vector<std::pair<std::string, JsonNode*>> pairs;

    JsonNodeObject() : JsonNode(JsonType::Object) {
    }

    virtual void print(std::ostream& os, int indent ) const {
        for( int i = 0; i < indent; i ++ )
            os<<"    ";
        os<<"{"<<std::endl;
        for( int i = 0; i < pairs.size(); i ++ )
        {
            std::pair<std::string, JsonNode*> pair = pairs[i];
            for( int i = 0; i < indent+1; i ++ )
                os<<"    ";
            os<<"\""<<pair.first<<"\":"<<std::endl;
            pair.second->print(os, indent+2);
            if( i != pairs.size()-1 )
                os<<","<<std::endl;
        }
        os<<std::endl;
        for( int i = 0; i < indent; i ++ )
            os<<"    ";
        os<<"}";
    }

    JsonNodeObject(const JsonNodeObject&) = delete;
    JsonNodeObject(JsonNodeObject&&) = delete;
    JsonNodeObject& operator=(const JsonNodeObject&) = delete;
    JsonNodeObject& operator=(JsonNodeObject&&) = delete;

    virtual ~JsonNodeObject() {
        for (const std::pair<std::string, JsonNode*>& p : pairs)
            delete p.second;
    }
};


class JsonNodeArray : public JsonNode {
public:
    std::vector<JsonNode*> array;

    JsonNodeArray() : JsonNode(JsonType::Array) {}

    virtual void print(std::ostream& os, int indent ) const {
        for( int i = 0; i < indent; i ++ )
            os<<"    ";
        os<<"["<<std::endl;
        for( int i = 0; i < array.size(); i ++ )
        {
            JsonNode* json = array[i];
            json->print(os, indent+1);
            if( i != array.size()-1 )
                os<<","<<std::endl;
        }
        os<<std::endl;
        for( int i = 0; i < indent; i ++ )
            os<<"    ";
        os<<"]";
    }

    JsonNodeArray(const JsonNodeArray&) = delete;
    JsonNodeArray(JsonNodeArray&&) = delete;
    JsonNodeArray& operator=(const JsonNodeArray&) = delete;
    JsonNodeArray& operator=(JsonNodeArray&&) = delete;

    virtual ~JsonNodeArray() {
        for (JsonNode* j : array)
            delete j;
    }
};


// Json string, Json number and Json error
class JsonNodeSingle : public JsonNode {
public:
    const std::string value;

    JsonNodeSingle(JsonType type, const std::string& value) : JsonNode(type), value(value) {}

    virtual void print(std::ostream& os, int indent ) const {
        for( int i = 0; i < indent; i ++ )
            os<<"    ";
        switch( type )
        {
            case JsonType::String:
                os<<"\""<<value<<"\"";
                break;
            case JsonType::Number:
                os<<value;
                break;
            case JsonType::Error:
                os<<"error: "<<value;
                break;
            default:
                os<<"error in JsonSingle::print";
        }
    }

    JsonNodeSingle(const JsonNodeSingle&) = delete;
    JsonNodeSingle(JsonNodeSingle&&) = delete;
    JsonNodeSingle& operator=(const JsonNodeSingle&) = delete;
    JsonNodeSingle& operator=(JsonNodeSingle&&) = delete;
    
    virtual ~JsonNodeSingle() = default;
};

// used when parsing JsonObject, should not appear in parsed result
class JsonNodeTmp : public JsonNode {
public:
    JsonNodeSingle* jsonStr;

    JsonNodeTmp() : JsonNode(JsonType::Tmp), jsonStr(nullptr) {}

    virtual void print(std::ostream& os, int indent ) const {
        for( int i = 0; i < indent; i ++ )
            os<<"    ";
        os<<"tmp: "<<std::endl;
        jsonStr->print(os, indent+1);
    }

    JsonNodeTmp(const JsonNodeTmp&) = delete;
    JsonNodeTmp(JsonNodeTmp&&) = delete;
    JsonNodeTmp& operator=(const JsonNodeTmp&) = delete;
    JsonNodeTmp& operator=(JsonNodeTmp&&) = delete;

    virtual ~JsonNodeTmp() {
        delete jsonStr;
    }
};


class Json {
public:
    std::unique_ptr<JsonNode> root;

    Json( JsonNode* j ) : root(j) {}

    Json(Json&&) = default;
    Json& operator=(Json&&) = default;

    Json(const Json&) = delete;
    Json& operator=(const Json&) = delete;

    virtual ~Json() = default;
};


std::ostream& operator<<(std::ostream&, const Json&);


class JsonParser final {
private:
    std::stack<JsonNode*> stack;
    std::stringstream buffer;

    enum class State { Start, Finished, Error, S1, S2, S3, S4};
    State state;
    JsonType current;
    JsonNode* result;

    int pushStack( JsonNode* );
    int pushStackError( JsonNode* );
    int pushStackError( char );

    int parseObject(char);
    int parseArray(char);
    int parseString(char);
    int parseNumber(char);
    int parseConstString( char, const char* );

    void give_id( JsonNode* node, int& count );


public:
    JsonParser() : state(State::Start), result(nullptr) {}

    // returns:
    //     1: finished;
    //     0: not finished;
    //     2: error
    // parse(0) will terminate parsing, and cause error if json not complete
    // but it's fine to put a 0 within a string. it will be part of the string
    int parse(char);

    Json get() {
        int count = 0;
        give_id(result, count);
        Json ret(result);
        result = nullptr;
        return ret;
    }

    ~JsonParser() {
        delete result;
    }
};


} // end of namespace json


#endif  // #ifndef JSON_JSON_H