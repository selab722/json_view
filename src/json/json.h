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


#ifdef JSON_JSON_IMPLEMENTATION

#include <cstring>
#include <cstdio>
#include <vector>
#include <regex>
#include <iostream>

#include "json/json.h"
#include "json/utils.h"

using std::vector;
using std::string;
using std::stringstream;
using std::ostream;
using std::pair;
using std::optional;
using std::hex;
using std::endl;
using std::cout;



namespace json {

ostream& operator<<(ostream& os, const JsonType& json ) {
    switch( json ) {
        case JsonType::Object:
            os<<"JsonType::Object";
            break;
        case JsonType::Array:
            os<<"JsonType::Object";
            break;
        case JsonType::String:
            os<<"JsonType::String";
            break;
        case JsonType::Number:
            os<<"JsonType::Number";
            break;
        case JsonType::Error:
            os<<"JsonType::Error";
            break;
        case JsonType::Tmp:
            os<<"JsonType::Tmp";
            break;
        case JsonType::True:
            os<<"JsonType::True";
            break;
        case JsonType::False:
            os<<"JsonType::False";
            break;
        case JsonType::Null:
            os<<"JsonType::Null";
            break;
        default:
            os<<"UnknownType";
    }
    return os;
}

ostream& operator<<(ostream& os, const Json& json ) {
    (json.root)->print(os, 0);
    return os;
}

void JsonParser::give_id( JsonNode* node, int& count ) {
    const int id = count++;
    node->id = id;

    if( typeid(*node) == typeid(JsonNodeObject) )
        for( pair<string, JsonNode*>& p : ((JsonNodeObject*)node)->pairs )
            give_id(p.second, count);
    else if( typeid(*node) == typeid(JsonNodeArray) )
        for (JsonNode*& child : ((JsonNodeArray*)node)->array)
            give_id(child, count);
}

int JsonParser::parse(char ch) {
    if (state == State::Error || state == State::Finished)
        return (state==State::Error) ? 2 : 1;

    if (state == State::Start) {
        if (std::isspace(static_cast<unsigned char>(ch)))
            return 0;
        else if (ch == '{') {
            stack.push(new JsonNodeObject());
            current = JsonType::Object;
        }
        else if (ch == '[') {
            stack.push(new JsonNodeArray());
            current = JsonType::Array;
        }
        else if (ch == '"')
            current = JsonType::String;
        else if (ch == '-' || (ch >= '0' && ch <= '9')) {
            current = JsonType::Number;
            buffer << ch;
        }
        else if( ch == 0 )  // this cannot swap with "else if" below
            pushStackError('0');
        else if( strchr("tfn", ch ) ) {
            buffer << ch;
            if (ch == 't')
                current = JsonType::True;
            else if (ch == 'f')
                current = JsonType::False;
            else
                current = JsonType::Null;
        }
        else
            return pushStackError(ch);
        state = State::S1;
        return 0;
    }

    switch (current) {
        case JsonType::Object:
            return parseObject(ch);
        case JsonType::Array:
            return parseArray(ch);
        case JsonType::String:
            return parseString(ch);
        case JsonType::Number:
            return parseNumber(ch);
        case JsonType::True:
            return parseConstString(ch, "true");
        case JsonType::False:
            return parseConstString(ch, "false");
        case JsonType::Null:
            return parseConstString(ch, "null");
        default:
            return pushStackError(ch);
    }
}

int JsonParser::pushStack(JsonNode* json) {
    if( stack.empty() ) {
        buffer.str("");
        state = State::Finished;
        result = json;
        return 1;
    }
    JsonNode* container = stack.top();
    switch( container->type) {
        case JsonType::Object:  // this means json->type == error
            return pushStackError(json);

        case JsonType::Array:
            static_cast<JsonNodeArray*>(container)->array.push_back(json);
            current = JsonType::Array;
            state = State::S2;
            return 0;

        case JsonType::Tmp:
        {
            JsonNodeTmp* tmp = static_cast<JsonNodeTmp*>(container);
            if( !(tmp->jsonStr) ) {
                if( json->type == JsonType::String ) {
                    tmp->jsonStr = static_cast<JsonNodeSingle*>(json);
                    current = JsonType::Object;
                    state = State::S2;
                }
                else // json->type == error
                    return pushStackError(json);
            }
            else {
                stack.pop();
                static_cast<JsonNodeObject*>(stack.top())->pairs.emplace_back(tmp->jsonStr->value, json);
                current = JsonType::Object;
                state = State::S3;
                delete tmp;
            }
            return 0;
        }

        default:
            return pushStackError(json);
    }
}

int JsonParser::pushStackError( JsonNode* error ) {
    while (!stack.empty()) {
        JsonNode* container = stack.top();
        switch( container->type ) {
            case JsonType::Object:
                static_cast<JsonNodeObject*>(container)->pairs.emplace_back("", error);
                break;
            case JsonType::Array:
                static_cast<JsonNodeArray*>(container)->array.push_back(error);
                current = JsonType::Array;
                break;

            case JsonType::Tmp:
            {
                JsonNodeTmp* tmp = static_cast<JsonNodeTmp*>(container);
                stack.pop();
                static_cast<JsonNodeObject*>(stack.top())->pairs.emplace_back((tmp->jsonStr)? (tmp->jsonStr->value) : "", error);
                delete tmp;
                break;
            }
            default: // severe error
                while( !stack.empty() ) {
                    delete stack.top();
                    stack.pop();
                }
                stack.push(error);
        }
        error = stack.top();
        stack.pop();
    }
    buffer.str("");
    result = error;
    state = State::Error;
    return 2;
}

int JsonParser::pushStackError( char ch ) {
    buffer<<ch;
    return pushStackError( new JsonNodeSingle( JsonType::Error, buffer.str()) );
}

int JsonParser::parseObject(char ch)
{
    // s1: wait for string or '}'; s2: wait for ':'; s3: wait for ',' or '}'; s4: wait for string.
    switch( state )
    {
        case State::S1:
            if( ch == '}' ) {
                JsonNode* obj = stack.top();
                stack.pop();
                return pushStack(obj);
            }
        case State::S4:
            if (std::isspace(static_cast<unsigned char>(ch)))
                return 0;
            if (ch == '"') {
                stack.push(new JsonNodeTmp());
                current = JsonType::String;
                state = State::S1;
                return 0;
            }
            return pushStackError(ch);

        case State::S2:
            if (std::isspace(static_cast<unsigned char>(ch)))
                return 0;
            if ( ch == ':' ) {
                state = State::Start;
                return 0;
            }
            return pushStackError(ch);

        case State::S3:
            if (std::isspace(static_cast<unsigned char>(ch)))
                return 0;
            if( ch == ',' ) {
                state = State::S4;
                return 0;
            }
            if( ch == '}' ) {
                JsonNode* obj = stack.top();
                stack.pop();
                return pushStack(obj);
            }
            return pushStackError(ch);

        default:
            return pushStackError(ch);
    }
}

int JsonParser::parseArray(char ch)
{
    // s1: wait for json or ']'; s2: wait for ',' or ']'.
    if( ch == ']' ) {
        JsonNode* array = stack.top();
        stack.pop();
        return pushStack(array);
    }

    if( state == State::S1 ) {
        state = State::Start;
        return parse(ch);
    }
    else if( ch == ',' ) {
        state = State::Start;
        return 0;
    }
    else
        return pushStackError(ch);

}

int JsonParser::parseString(char ch) {
    // s1: parse string; see '"' as the end, see '\\' as escape
    // s2: after parsed '\\', don't see '"' as the end
    switch( state ) {
        case State::S1:
            if( ch == '"' ) {
                int result = 0;
                const string rawString = buffer.str();
                buffer.str("");
                optional<string> converted = ConvertUtf16::handle_escape(rawString);//TODO
                return converted.has_value() ?
                    pushStack(new JsonNodeSingle(JsonType::String, converted.value())) :
                    pushStackError(new JsonNodeSingle(JsonType::Error, rawString));
            }
            else if( ch == '\\' ) {
                buffer << '\\';
                state = State::S2;
                return 0;
            }
            else {
                buffer << ch;
                return 0;
            }

        case State::S2:
            buffer << ch;
            state = State::S1;
            return 0;

        default:
            return pushStackError(ch);
    }
}


int JsonParser::parseNumber(char ch) {
    const static std::regex numberRegex("-?(0|([1-9]\\d*))(\\.\\d+)?([eE][-\\+]?\\d+)?");
    if( ch != 0 && strchr("0123456789-.eE", ch ) ) {
        buffer << ch;
        return 0;
    }
    const string numberStr = buffer.str();
    buffer.str("");
    if( !std::regex_match(numberStr, numberRegex) )
        return pushStackError(new JsonNodeSingle(JsonType::Error, numberStr));
    int result = pushStack(new JsonNodeSingle(JsonType::Number, numberStr));
    if( state != State::Finished )
        return parse(ch);
    else
        return result;
}

int JsonParser::parseConstString( char ch, const char* str ) {
    if( ch != str[buffer.str().size()] )
        return pushStackError(ch);
    buffer << ch;
    if( buffer.str().size() == strlen(str) ) {
        switch( buffer.str()[0] ) {
            case 't':
                buffer.str("");
                return pushStack(new JsonNode(JsonType::True));
            case 'f':
                buffer.str("");
                return pushStack(new JsonNode(JsonType::False));
            case 'n':
                buffer.str("");
                return pushStack(new JsonNode(JsonType::Null));
            default:
                return pushStackError(new JsonNodeSingle(JsonType::Error, buffer.str()));
        }
    }

    return 0;
}

}

#endif  // #ifdef JSON_JSON_IMPLEMENTATION

#endif  // #ifndef JSON_JSON_H