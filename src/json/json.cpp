#include "json/json.h"

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
