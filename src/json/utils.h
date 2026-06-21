#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <optional>
#include <string>
#include <vector>
#include <sstream>

namespace json {

// convert string like "View from 15th Floor\uD834\uDD1E" to utf8 string
class ConvertUtf16 {
private:

char static hexDigit( char ch ) {
    if (ch<='9' && ch>='0')
        return ch - '0';
    if (ch<='F' && ch>='A')
        return ch-'A'+(char)10;
    if (ch<='f' && ch>='a')
        return ch-'a'+(char)10;
    return -1;
}

void static utf16ToUtf8(std::stringstream& ss, char16_t& high, char16_t c ) {
    if (high) {
        high = (high & 0x3FF)+0x40;
        c = c & 0x3FF;
        ss << static_cast<char>(0xF0 | (high>>8))
           << static_cast<char>(0x80 | ((high>>2) & 0x3F))
           << static_cast<char>(0x80 | ((high&3)<<4) | (c>>6))
           << static_cast<char>(0x80 | (c & 0x3f));
        high = 0;
        return;
    }
    if (c <= 0x7F) {
        ss << static_cast<char>(c);
    } else if (c <= 0x7FF) {
        ss << static_cast<char>(0xC0 | (c >> 6))
           << static_cast<char>(0x80 | (c & 0x3F));
    } else if ( c>= 0xd800 && c <= 0xdbff ) {
        high = c;
    } else {
        ss << static_cast<char>(0xE0 | (c >> 12))
           << static_cast<char>(0x80 | ((c >> 6) & 0x3F))
           << static_cast<char>(0x80 | (c & 0x3F));
    }
}


public:
    std::optional<std::string> static handle_escape( const std::string& rawString ) {

        std::stringstream buffer;

        const size_t len = rawString.size();

        for( int i = 0; i < len; i ++ ) {
            char cur = rawString[i];
            if( cur != '\\' ) {
                if( cur == '"' )
                    return std::nullopt;
                buffer << cur;
                continue;
            }

            if( len > i+5 && rawString[i]=='\\' && rawString[i+1]=='u' ) {
                char16_t high = 0;
                i--;
                do {
                    i += 2;
                    char16_t utf16Char = 0;
                    for( int j = 0; j < 4; j ++ ) {
                        char digit = hexDigit(rawString[++i]);
                        if( digit == -1 )
                            return std::nullopt;
                        utf16Char = (utf16Char<<4) | (unsigned char)digit;
                    }
                    utf16ToUtf8(buffer, high, utf16Char);
                } while( len > i+6 && rawString[i+1]=='\\' && rawString[i+2]=='u' );
                continue;
            }

            if( len <= ++i )
                return std::nullopt;
            cur = rawString[i];
            switch( cur ) {
                case '/':
                case '\\':
                case '"':
                    buffer<<cur;
                    break;
                case 'b':
                    buffer<<'\b';
                    break;
                case 'f':
                    buffer<<'\f';
                    break;
                case 'n':
                    buffer<<'\n';
                    break;
                case 'r':
                    buffer<<'\r';
                    break;
                case 't':
                    buffer<<'\t';
                    break;

                default:
                    return std::nullopt;
            }
        }
        return buffer.str();
    }
};


}

#endif